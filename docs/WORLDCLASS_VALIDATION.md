# HeartLake 评审级验证报告

- 生成时间: 2026-02-23T19:49:05Z
- 基础地址: `http://127.0.0.1:8080`

## 1) 功能回归
- 功能通过率: **30/30 (100.00%)**
- 失败数: **0**

## 2) 安全与契约
- 安全检查通过率: **6/6 (100.00%)**
- 数据一致性通过率: **5/5 (100.00%)**

## 3) 情感精度
- Mood Top-1 准确率: **96.88%**
- Mood Macro-F1: **97.08%**
- 极性准确率: **100.00%**
- 关键句命中率: **5/5 (100.00%)**

## 4) 压力与稳定性
### read_heavy / 并发 16
- 时长: 20s
- RPS: **1030.75**
- 错误率: **0.00%**
- 延迟: p50=6.54ms, p95=53.74ms, p99=85.98ms, avg=15.51ms
- 压测后健康检查: **True**

### ai_mixed / 并发 16
- 时长: 20s
- RPS: **1019.45**
- 错误率: **0.00%**
- 延迟: p50=6.97ms, p95=52.32ms, p99=85.06ms, avg=15.69ms
- 压测后健康检查: **True**

### read_heavy / 并发 32
- 时长: 20s
- RPS: **1212.4**
- 错误率: **0.00%**
- 延迟: p50=26.32ms, p95=28.55ms, p99=30.35ms, avg=26.39ms
- 压测后健康检查: **True**

### ai_mixed / 并发 32
- 时长: 20s
- RPS: **1221.55**
- 错误率: **0.00%**
- 延迟: p50=26.2ms, p95=28.67ms, p99=30.04ms, avg=26.2ms
- 压测后健康检查: **True**

### read_heavy / 并发 64
- 时长: 20s
- RPS: **1228.4**
- 错误率: **0.00%**
- 延迟: p50=51.9ms, p95=55.71ms, p99=58.36ms, avg=52.11ms
- 压测后健康检查: **True**

### ai_mixed / 并发 64
- 时长: 20s
- RPS: **1172.3**
- 错误率: **0.00%**
- 延迟: p50=54.41ms, p95=59.69ms, p99=65.43ms, avg=54.6ms
- 压测后健康检查: **True**

## 5) QUIC 通道
- UDP 发送探测: **True** (`127.0.0.1:8443`)

## 6) 严格门槛判定
- 通过门槛: **11/11 (100.00%)**
- 结论: **门槛全部通过：当前样本集下达到评审级稳定标准**

- `functional_pass_rate`: actual=1.0 target=0.99 pass=True
- `security_pass_rate`: actual=1.0 target=1.0 pass=True
- `consistency_pass_rate`: actual=1.0 target=1.0 pass=True
- `mood_accuracy`: actual=0.9688 target=0.7 pass=True
- `macro_f1`: actual=0.9708 target=0.68 pass=True
- `polarity_accuracy`: actual=1.0 target=0.9 pass=True
- `critical_case_hit_rate`: actual=1.0 target=1.0 pass=True
- `stress_error_rate_max`: actual=0.0 target=0.01 pass=True
- `stress_p95_ms_max`: actual=59.69 target=800.0 pass=True
- `stress_health_all_ok`: actual=True target=True pass=True
- `quic_udp_send_ok`: actual=True target=True pass=True

## 说明
- 该报告提供“可复现证据链”，但无法在单机本地直接证明“优于世界上所有项目”。
- 若要形成国际评审级对外论证，需补齐跨机器、跨地区、公开基线对比的第三方实验。
