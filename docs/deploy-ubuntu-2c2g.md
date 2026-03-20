# HeartLake Ubuntu 2C2G Lite 部署

适用场景：

- Ubuntu 单机云服务器
- 2 vCPU / 2 GiB / 40 GiB 左右
- 不部署 Ollama
- 统一入口使用 Nginx gateway

## 1. 云控制台需要确认的内容

- 安全组放行 `22`
- 安全组放行 `80`
- 如果后面要上 HTTPS，再放行 `443`
- 如果已有域名，将 A 记录解析到服务器公网 IP

## 2. 服务器初始化

```bash
sudo apt update
sudo apt install -y docker.io docker-compose-v2 git openssl curl
sudo systemctl enable --now docker
sudo usermod -aG docker "$USER"
```

重新登录一次 shell，让 `docker` 组生效。

低配机器建议加 2G swap，避免后端 Docker 构建阶段 OOM：

```bash
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

当前 `server-lite` 模板已默认关闭 ONNX 线程放大，把 Redis 连接池上限压到 `8`，并默认关闭湖神守护 / 情绪追踪 / 用户回访这三类后台扫描任务，更适合 `2C2G` 单机。

注意：

- `2C2G` 机器不要把“只改后台前端”也用成 `docker compose ... up -d --build admin`
- 这类命令在 Compose 依赖链下可能把 `backend` 也一起带进构建，云机容易进入持续内存压力，严重时 SSH/HTTP 会一起卡死
- 全量升级才用 `./scripts/docker-up.sh server-lite`
- 只改管理后台时改用 `./scripts/docker-up.sh server-lite-admin`
- 只改后端时改用 `./scripts/docker-up.sh server-lite-backend`
- 只改网关配置时改用 `./scripts/docker-up.sh server-lite-gateway`

## 3. 拉代码并生成服务器环境文件

```bash
git clone <你的仓库地址>
cd HeartLake
./scripts/generate-server-env.sh --host 121.41.195.165 --admin-password '替换成强密码'
cp .env.server-lite .env
```

如果已经有域名并且准备用 HTTPS，可以这样生成：

```bash
./scripts/generate-server-env.sh --host your.domain.com --https --admin-password '替换成强密码'
cp .env.server-lite .env
```

说明：

- `server-lite` 模板现在默认按“外部 OpenAI-compatible API”口径填写 AI 参数，更适合 `2C2G`
- 如果你要改回本机 `ollama`，请手动把 `AI_PROVIDER=ollama`，并确认 `AI_BASE_URL` 在服务器内可达
- 三类后台扫描任务默认关闭；确认机器余量后再逐项改为 `true`

## 4. 启动精简部署

```bash
docker compose pull postgres redis gateway || true
./scripts/docker-up.sh server-lite
```

如果只是增量更新，优先用更轻的命令：

```bash
# 只改 admin
./scripts/docker-up.sh server-lite-admin

# 只改 backend
./scripts/docker-up.sh server-lite-backend

# 只改 gateway
./scripts/docker-up.sh server-lite-gateway
```

## 5. 冒烟验证

```bash
curl -fsS http://127.0.0.1/healthz
curl -fsS http://127.0.0.1/api/health
curl -I http://127.0.0.1/admin/
```

如果只做轻量烟测，避免跑整套历史测试：

```bash
GATEWAY_PORT=80 RUN_EXISTING_TESTS=0 ./scripts/docker-test.sh
```

如果要做论文验收或交付前全量验证，直接跑统一的 2C2G 标准脚本：

```bash
./scripts/verify-2c2g.sh
```

如果需要恢复中文情绪 ONNX 模型，再执行：

```bash
./scripts/restore-onnx-model.sh
```

如果需要验证 AI 模块已经真实加载 ONNX Runtime 与模型文件，再执行：

```bash
VERIFY_ONNX_SMOKE=1 ./scripts/verify-onnx-smoke.sh
```

## 6. 访问入口

- 管理后台：`http://服务器IP/admin/`
- API：`http://服务器IP/api/`
- WebSocket：`ws://服务器IP/ws/broadcast`

## 7. 后续升级到 HTTPS

当前仓库里的 gateway 已补齐 `/ws/` 代理，可以直接在宿主机前面再接一层证书终止，或把证书配置加进 gateway。

在未配域名和证书之前，建议先用 IP + HTTP 跑通。
