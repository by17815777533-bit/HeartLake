use anyhow::{anyhow, Context, Result};
use base64::{engine::general_purpose::URL_SAFE_NO_PAD, Engine as _};
use chacha20poly1305::{aead::Aead, KeyInit};
use chrono::{NaiveDateTime, Utc};
use futures_util::StreamExt;
use quinn::{Connection, Endpoint, ServerConfig};
use rustls::pki_types::{CertificateDer, PrivateKeyDer, PrivatePkcs8KeyDer};
use serde::Deserialize;
use std::collections::HashMap;
use std::net::SocketAddr;
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::{RwLock, Semaphore};
use tracing::{error, info, warn};

type SessionMap = Arc<RwLock<HashMap<String, HashMap<usize, Connection>>>>;

const REALTIME_CHANNEL: &str = "heartlake:realtime";

#[derive(Debug, Deserialize)]
struct AuthPacket {
    #[serde(default)]
    r#type: String,
    token: String,
}

#[derive(Debug, Deserialize)]
struct PasetoPayload {
    sub: String,
    exp: String,
}

#[derive(Debug, Deserialize)]
struct RealtimeEnvelope {
    scope: String,
    #[serde(default)]
    target: String,
    payload: String,
}

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(
            std::env::var("QUIC_GATEWAY_LOG")
                .unwrap_or_else(|_| "info,quinn=warn,hyper=warn".to_string()),
        )
        .init();

    let bind = std::env::var("QUIC_GATEWAY_BIND").unwrap_or_else(|_| "0.0.0.0:8443".to_string());
    let bind_addr: SocketAddr = bind
        .parse()
        .with_context(|| format!("invalid QUIC_GATEWAY_BIND: {bind}"))?;

    let paseto_key = read_paseto_key()?;
    let redis_url = build_redis_url();
    let sessions: SessionMap = Arc::new(RwLock::new(HashMap::new()));

    let endpoint = start_quic_server(bind_addr)?;
    info!("QUIC gateway listening on {bind_addr}");

    // 最大并发连接数限制，防止资源耗尽
    let max_conns: usize = std::env::var("QUIC_MAX_CONNECTIONS")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(1000);
    let conn_semaphore = Arc::new(Semaphore::new(max_conns));

    {
        let sessions = sessions.clone();
        let redis_url = redis_url.clone();
        tokio::spawn(async move {
            if let Err(e) = run_redis_fanout(redis_url, sessions).await {
                error!("redis fanout stopped: {e:#}");
            }
        });
    }

    while let Some(connecting) = endpoint.accept().await {
        let sessions = sessions.clone();
        let paseto_key = paseto_key;
        let sem = conn_semaphore.clone();
        tokio::spawn(async move {
            // 获取许可，连接结束时自动释放
            let _permit = match sem.acquire_owned().await {
                Ok(p) => p,
                Err(_) => {
                    warn!("connection semaphore closed, rejecting");
                    return;
                }
            };
            match connecting.await {
                Ok(conn) => {
                    if let Err(e) = handle_connection(conn, sessions, paseto_key).await {
                        warn!("connection handler error: {e:#}");
                    }
                }
                Err(e) => warn!("incoming QUIC handshake failed: {e}"),
            }
        });
    }

    Ok(())
}

fn read_paseto_key() -> Result<[u8; 32]> {
    let key = std::env::var("PASETO_KEY").context("PASETO_KEY is required for QUIC gateway auth")?;
    // 优先尝试 base64 解码，失败则当作原始字节
    let decoded = base64::engine::general_purpose::STANDARD
        .decode(&key)
        .or_else(|_| URL_SAFE_NO_PAD.decode(&key))
        .unwrap_or_else(|_| key.into_bytes());
    if decoded.len() < 32 {
        return Err(anyhow!(
            "PASETO_KEY too short: got {} bytes, need at least 32",
            decoded.len()
        ));
    }
    let mut out = [0_u8; 32];
    out.copy_from_slice(&decoded[..32]);
    Ok(out)
}

fn build_redis_url() -> String {
    if let Ok(url) = std::env::var("REDIS_URL") {
        if !url.trim().is_empty() {
            return url;
        }
    }
    let host = std::env::var("REDIS_HOST").unwrap_or_else(|_| "127.0.0.1".to_string());
    let port = std::env::var("REDIS_PORT").unwrap_or_else(|_| "6379".to_string());
    let password = std::env::var("REDIS_PASSWORD").unwrap_or_default();
    if password.is_empty() {
        format!("redis://{host}:{port}/")
    } else {
        format!("redis://:{password}@{host}:{port}/")
    }
}

fn start_quic_server(bind_addr: SocketAddr) -> Result<Endpoint> {
    let server_config = build_server_config()?;
    let endpoint = Endpoint::server(server_config, bind_addr)?;
    Ok(endpoint)
}

fn build_server_config() -> Result<ServerConfig> {
    let (cert_der, key_der) = load_tls_identity()?;

    let mut server_config = ServerConfig::with_single_cert(vec![cert_der], key_der)?;
    let mut transport_config = quinn::TransportConfig::default();
    // 限制单向流并发数，防止资源耗尽攻击
    transport_config.max_concurrent_uni_streams(32_u32.into());
    transport_config.max_concurrent_bidi_streams(64_u32.into());
    transport_config.keep_alive_interval(Some(Duration::from_secs(20)));
    transport_config.max_idle_timeout(Some(Duration::from_secs(120).try_into()?));
    server_config.transport_config(Arc::new(transport_config));
    Ok(server_config)
}

/// 加载 TLS 证书和私钥。优先从 QUIC_TLS_CERT / QUIC_TLS_KEY 环境变量指定的文件读取，
/// 若未配置则回退到自签名证书（仅适用于开发环境）。
fn load_tls_identity() -> Result<(CertificateDer<'static>, PrivateKeyDer<'static>)> {
    let cert_path = std::env::var("QUIC_TLS_CERT").ok();
    let key_path = std::env::var("QUIC_TLS_KEY").ok();

    if let (Some(cp), Some(kp)) = (cert_path, key_path) {
        let cert_pem = std::fs::read(&cp)
            .with_context(|| format!("failed to read TLS cert from {cp}"))?;
        let key_pem = std::fs::read(&kp)
            .with_context(|| format!("failed to read TLS key from {kp}"))?;

        let cert = rustls_pemfile::certs(&mut &cert_pem[..])
            .next()
            .ok_or_else(|| anyhow!("no certificate found in {cp}"))?
            .context("failed to parse certificate PEM")?;
        let key = rustls_pemfile::private_key(&mut &key_pem[..])
            .context("failed to parse private key PEM")?
            .ok_or_else(|| anyhow!("no private key found in {kp}"))?;

        info!("loaded TLS identity from {cp} / {kp}");
        return Ok((cert, key));
    }

    warn!("QUIC_TLS_CERT / QUIC_TLS_KEY not set — using self-signed certificate (dev only)");
    let self_signed = rcgen::generate_simple_self_signed(vec![
        "localhost".to_string(),
        "127.0.0.1".to_string(),
    ])?;
    let cert_der = CertificateDer::from(self_signed.cert.der().to_vec());
    let key_der = PrivateKeyDer::Pkcs8(PrivatePkcs8KeyDer::from(
        self_signed.key_pair.serialize_der(),
    ));
    Ok((cert_der, key_der))
}

async fn handle_connection(conn: Connection, sessions: SessionMap, paseto_key: [u8; 32]) -> Result<()> {
    let conn_id = conn.stable_id();
    let remote = conn.remote_address();
    info!("new QUIC connection id={conn_id} from {remote}");

    let (mut send, mut recv) = conn.accept_bi().await.context("auth bi-stream required")?;
    let auth_bytes = recv
        .read_to_end(8 * 1024)
        .await
        .context("read auth packet failed")?;
    let auth_text = String::from_utf8(auth_bytes).context("auth packet must be UTF-8")?;
    let packet: AuthPacket = serde_json::from_str(&auth_text).context("invalid auth packet json")?;
    if packet.r#type != "auth" {
        send_error(&mut send, "first packet must be auth").await?;
        return Err(anyhow!("invalid first packet type"));
    }

    let user_id = verify_paseto_token(&packet.token, paseto_key)?;
    {
        let mut guard = sessions.write().await;
        guard
            .entry(user_id.clone())
            .or_default()
            .insert(conn_id, conn.clone());
    }

    let ok = serde_json::json!({
        "type": "auth_ok",
        "user_id": user_id,
        "transport": "quic"
    });
    send.write_all(ok.to_string().as_bytes()).await?;
    send.finish()?;

    conn.closed().await;
    {
        let mut guard = sessions.write().await;
        if let Some(conns) = guard.get_mut(&user_id) {
            conns.remove(&conn_id);
            if conns.is_empty() {
                guard.remove(&user_id);
            }
        }
    }
    info!("QUIC connection closed id={conn_id}");
    Ok(())
}

async fn send_error(send: &mut quinn::SendStream, message: &str) -> Result<()> {
    let err = serde_json::json!({
        "type": "auth_error",
        "message": message
    });
    send.write_all(err.to_string().as_bytes()).await?;
    send.finish()?;
    Ok(())
}

fn verify_paseto_token(token: &str, key_material: [u8; 32]) -> Result<String> {
    const HEADER: &str = "v4.local.";
    if !token.starts_with(HEADER) {
        return Err(anyhow!("invalid token header"));
    }
    let raw = URL_SAFE_NO_PAD
        .decode(&token[HEADER.len()..])
        .context("invalid base64url token body")?;
    if raw.len() < 12 + 16 {
        return Err(anyhow!("token body too short"));
    }

    let nonce = chacha20poly1305::Nonce::from_slice(&raw[..12]);
    let cipher = chacha20poly1305::ChaCha20Poly1305::new((&key_material).into());
    let plaintext = cipher
        .decrypt(nonce, &raw[12..])
        .map_err(|_| anyhow!("token decrypt failed"))?;

    let payload: PasetoPayload = serde_json::from_slice(&plaintext).context("token payload json invalid")?;
    let exp = NaiveDateTime::parse_from_str(&payload.exp, "%Y-%m-%dT%H:%M:%SZ")
        .context("token exp parse failed")?;
    let exp_utc = exp.and_utc();
    if Utc::now() > exp_utc {
        return Err(anyhow!("token expired"));
    }
    if payload.sub.trim().is_empty() {
        return Err(anyhow!("empty user id in token"));
    }

    Ok(payload.sub)
}

async fn run_redis_fanout(redis_url: String, sessions: SessionMap) -> Result<()> {
    let mut backoff_secs = 2_u64;
    const MAX_BACKOFF: u64 = 60;
    loop {
        match run_redis_fanout_once(&redis_url, &sessions).await {
            Ok(()) => {
                warn!("redis fanout exited normally, reconnecting");
                // 正常退出说明连接曾经成功，重置退避
                backoff_secs = 2;
            }
            Err(e) => {
                warn!("redis fanout error: {e:#}, retrying in {backoff_secs}s");
            }
        }
        tokio::time::sleep(Duration::from_secs(backoff_secs)).await;
        backoff_secs = (backoff_secs * 2).min(MAX_BACKOFF);
    }
}

async fn run_redis_fanout_once(redis_url: &str, sessions: &SessionMap) -> Result<()> {
    let client = redis::Client::open(redis_url).context("open redis client failed")?;
    let mut pubsub = client.get_async_pubsub().await.context("create redis pubsub failed")?;
    pubsub
        .subscribe(REALTIME_CHANNEL)
        .await
        .context("subscribe realtime channel failed")?;
    info!("subscribed redis channel: {REALTIME_CHANNEL}");

    let mut stream = pubsub.on_message();
    while let Some(msg) = stream.next().await {
        let payload: String = msg.get_payload().context("redis payload decode failed")?;
        if let Err(e) = fanout_event(&payload, sessions).await {
            warn!("fanout event skipped: {e:#}");
        }
    }
    Ok(())
}

async fn fanout_event(event_text: &str, sessions: &SessionMap) -> Result<()> {
    let envelope: RealtimeEnvelope =
        serde_json::from_str(event_text).context("invalid realtime envelope json")?;
    let payload = envelope.payload.into_bytes();

    let mut stale = Vec::new();
    let targets: Vec<(String, usize, Connection)> = {
        let guard = sessions.read().await;
        match envelope.scope.as_str() {
            "broadcast" => guard
                .iter()
                .flat_map(|(uid, m)| {
                    m.iter()
                        .map(move |(id, conn)| (uid.clone(), *id, conn.clone()))
                })
                .collect(),
            "user" => guard
                .get(&envelope.target)
                .map(|m| {
                    m.iter()
                        .map(|(id, conn)| (envelope.target.clone(), *id, conn.clone()))
                        .collect()
                })
                .unwrap_or_default(),
            "room" => {
                let user_ids = users_from_room(&envelope.target);
                user_ids
                    .into_iter()
                    .flat_map(|uid| {
                        guard
                            .get(&uid)
                            .map(|m| {
                                m.iter()
                                    .map(|(id, conn)| (uid.clone(), *id, conn.clone()))
                                    .collect::<Vec<_>>()
                            })
                            .unwrap_or_default()
                    })
                    .collect()
            }
            _ => Vec::new(),
        }
    };

    for (uid, conn_id, conn) in targets {
        if let Err(e) = send_to_connection(&conn, &payload).await {
            warn!("push to QUIC client failed uid={uid} conn={conn_id}: {e}");
            stale.push((uid, conn_id));
        }
    }

    if !stale.is_empty() {
        let mut guard = sessions.write().await;
        for (uid, conn_id) in stale {
            if let Some(m) = guard.get_mut(&uid) {
                m.remove(&conn_id);
                if m.is_empty() {
                    guard.remove(&uid);
                }
            }
        }
    }

    Ok(())
}

fn users_from_room(room: &str) -> Vec<String> {
    if let Some(rest) = room.strip_prefix("private:") {
        return rest
            .split('_')
            .filter(|s| !s.trim().is_empty())
            .map(|s| s.to_string())
            .collect();
    }
    if let Some(uid) = room.strip_prefix("user:") {
        if !uid.trim().is_empty() {
            return vec![uid.to_string()];
        }
    }
    Vec::new()
}

async fn send_to_connection(conn: &Connection, payload: &[u8]) -> Result<()> {
    let mut stream = conn.open_uni().await.context("open uni stream failed")?;
    stream.write_all(payload).await.context("write stream failed")?;
    stream.finish()?;
    Ok(())
}
