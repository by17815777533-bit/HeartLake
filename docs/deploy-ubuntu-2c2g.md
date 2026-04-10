# Ubuntu 2C2G 部署手册

本文记录当前单机 `server-lite` 部署方式，适用于 HeartLake 现网结构。

## 1. 当前部署拓扑

```text
+-----------------+      +---------------------+      +----------------------+
| 公网流量        |----->| 121.41.195.165      |----->| gateway              |
+-----------------+      +---------------------+      +----------+-----------+
                                                             |      |
                                          +------------------+      +------------------+
                                          |                                        |
                                          v                                        v
                                 +----------------------+                 +----------------------+
                                 | heartlake-backend    |                 | heartlake-admin      |
                                 +----------+-----------+                 +----------------------+
                                            |
                         +------------------+------------------+
                         |                                     |
                         v                                     v
                +----------------------+             +----------------------+
                | heartlake-postgres   |             | heartlake-redis      |
                +----------------------+             +----------------------+
```

## 2. 目标环境

- Ubuntu
- 2 vCPU / 2 GiB RAM
- Docker + Docker Compose
- 仓库目录：`/root/HeartLake`

## 3. 服务器初始化

```bash
sudo apt update
sudo apt install -y docker.io docker-compose-v2 git openssl curl
sudo systemctl enable --now docker
sudo usermod -aG docker "$USER"
```

建议预留 2G swap：

```bash
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

## 4. 拉代码并生成环境文件

```bash
git clone <仓库地址> /root/HeartLake
cd /root/HeartLake
./scripts/generate-server-env.sh --host 121.41.195.165 --admin-password '<强密码>'
cp .env.server-lite .env
```

## 5. 启动与增量部署

### 全量启动

```bash
cd /root/HeartLake
./scripts/docker-up.sh server-lite
```

### 增量更新

```bash
./scripts/docker-up.sh server-lite-backend-local heartlake-server
./scripts/docker-up.sh server-lite-admin-local heartlake-server
./scripts/docker-up.sh server-lite-gateway
```

### 本地构建后推送到服务器

```bash
./scripts/deploy-server-lite-local.sh backend heartlake-server
./scripts/deploy-server-lite-local.sh admin heartlake-server
./scripts/deploy-server-lite-local.sh all heartlake-server
```

`server-lite` 和 `server-lite-admin` 这两个“服务器现编 admin”入口现在默认受保护。
在 2C2G 机器上它们容易把 `ssh` / `nginx` 一起拖死；除非显式设置 `HEARTLAKE_ALLOW_REMOTE_ADMIN_BUILD=1`，否则请始终走本地构建再推送。

## 6. 文档同步

```bash
rsync -az README.md docs admin/README.md backend/README.md frontend/README.md \
  CONTRIBUTING.md backend/models/README.md backend/migrations/README.md \
  datasets/CLUEREADME.md heartlake-server:/root/HeartLake/
```

## 7. 健康检查

### 服务器内

```bash
curl -fsS http://127.0.0.1/api/health
curl -fsS http://127.0.0.1/healthz
curl -I -fsS http://127.0.0.1/admin/
docker compose ps
```

### 公网

```bash
curl -fsS http://121.41.195.165/api/health
curl -fsS http://121.41.195.165/healthz
curl -I -fsS http://121.41.195.165/admin/
```

## 8. 上线后验证

```bash
cd /root/HeartLake
./scripts/docker-test.sh
./scripts/verify-2c2g.sh
```

如果需要验证 ONNX 链路：

```bash
cd /root/HeartLake
VERIFY_ONNX_SMOKE=1 ./scripts/verify-onnx-smoke.sh
```

## 9. 当前注意事项

- release 包默认直连 `http://121.41.195.165`
- WebSocket 统一使用 `ws://121.41.195.165/ws/broadcast`
- `server-lite` 是当前生产模式
- 部署后先看 `api/health`，再看 `/admin/`
