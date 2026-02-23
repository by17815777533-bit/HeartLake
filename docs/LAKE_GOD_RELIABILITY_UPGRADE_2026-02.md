# 湖神链路可靠性升级（2026-02）

## 目标
- 修复湖神重复模板回复、情绪脉搏偏差、守护者激励失效、自动加好友失效、情绪日历/趋势失效。
- 保持现有 C++/Flutter 分层与 API 路径兼容，不混写头文件和实现文件。

## 本次落地改动

### 1. 湖神回复去模板化
- 在 `backend/src/infrastructure/ai/DualMemoryRAG.cpp` 增加“动态本地回退”：
  - 当外部模型超时/报错/返回通用模板时，按 `当前情绪 + 长期趋势 + 当前话题片段` 生成差异化回复。
  - 去除固定句式兜底，避免持续输出同一句。

### 2. 情绪输出稳健化与口径统一
- 在 `backend/src/infrastructure/ai/AIService.cpp`：
  - 情绪结果增加 mood 归一化（如 `joy -> happy`、`fear -> anxious`）。
  - 情绪分析请求改为结构化 `response_format.type=json_object`，降低解析失败率。
- 在 `backend/src/interfaces/api/EdgeAIController.cpp`：
  - `GET /api/edge-ai/emotion-pulse` 增加 `normalized_score` 与 `trend` 字段。
  - `POST /api/edge-ai/emotion-sample` 支持 `score_scale=zero_one` 向后兼容。
  - 湖神聊天成功后同步提交情绪样本，确保脉搏实时更新。

### 3. 自动加好友修复
- 在 `backend/src/interfaces/api/PaperBoatController.cpp`：
  - 临时好友字段从错误的 `user_id_1/user_id_2` 修正为 `user1_id/user2_id`。
  - 恢复纸船互动触发的自动临时好友关系。

### 4. 守护者激励链路恢复
- 在 `backend/src/application/InteractionApplicationService.cpp`：
  - 创建涟漪时记录 `quality_ripple`。
  - 发送纸船时记录 `warm_boat`。
- 在 `backend/src/interfaces/api/PaperBoatController.cpp`：
  - 纸船相关入口统一触发 `recordWarmBoat`，修复“温暖纸船”统计。
- 在 `backend/src/infrastructure/services/GuardianIncentiveService.cpp`：
  - `warm_boat` 积分计算做最小值保护，避免因低分被截断为 0。

### 5. 情绪日历/趋势与灯火判断修复
- 在 `backend/src/interfaces/api/UserController.cpp`：
  - 情绪日历兼容 `year + month` 参数。
  - 日历/热力图统计从 `stone_color` 切换为 `mood_type + emotion_score`。
- 在 `backend/src/interfaces/api/RecommendationController.cpp`：
  - 趋势统计改为基于 `stones` 的真实情绪分，缺失时按 mood 回退估算。
- 在 `backend/src/infrastructure/services/VIPService.cpp`：
  - VIP 自动关怀阈值查询列修正为 `avg_emotion_score`。

### 6. 角色与文案统一
- 前端统一“点灯人”到“守护者”，用户可见“AI”文案改为“湖神”：
  - `frontend/lib/presentation/screens/guardian_screen.dart`
  - `frontend/lib/presentation/screens/profile_screen.dart`
  - `frontend/lib/presentation/screens/onboarding_screen.dart`
  - `frontend/lib/presentation/screens/lake_god_chat_screen.dart`
  - `frontend/lib/presentation/screens/discover_screen.dart`
  - `frontend/lib/presentation/screens/emotion_trends_screen.dart`
  - `frontend/lib/presentation/widgets/ai_content_preview.dart`
  - `frontend/lib/presentation/widgets/emotion_insights_card.dart`
  - `frontend/lib/presentation/screens/publish_screen.dart`

### 7. ONNX 小模型启用兼容增强
- 在 `backend/src/infrastructure/ai/EdgeAIEngine.cpp`：
  - 保留显式开关优先级：`EDGE_AI_ONNX_ENABLED=true/false`。
  - 当开关未设置时，自动探测 `model + vocab` 文件并自动启用 ONNX。
  - `EDGE_AI_MODEL_PATH` 支持目录写法（如 `./models`），自动解析到 `sentiment_zh.onnx`。
  - `EDGE_AI_VOCAB_PATH` 未设置时，默认使用模型目录下 `vocab.txt`。
  - 增强日志：明确未启用原因（缺模型/缺词表/开关禁用）。
- 在 `backend/src/interfaces/api/EdgeAIController.cpp`：
  - 管理配置返回新增 `onnx_enabled/vocab_path/onnx_compiled` 字段，便于排查部署问题。

### 8. 论文引用核验与本次落地（在你已有引用基础上补强）
- 你仓库里已有 `papers/literature_review.md` 的大规模引用清单，本次没有重做体系，只补“可验证 + 可落地”映射：
  - `backend/src/infrastructure/ai/EdgeAIEngine.cpp`：
    - 参考 **Ada-EF** 思路加入 `HNSW` 查询阶段自适应 `ef`（先做小规模 pilot 再调节 `ef`），在易样本降开销、难样本提召回。
    - 参考 **AER-LLM（歧义情绪）**，将情绪样本写入脉搏时引入 `confidence` 权重，降低低置信噪声对脉搏和趋势的污染。
  - `backend/src/infrastructure/ai/RecommendationEngine.cpp`：
    - 参考 **多样性检索/重排** 研究，在 `MMR` 中加入作者重复惩罚，缓解同一作者/高热内容挤占。
    - 将探索查询从 `ORDER BY RANDOM()` 改为“时间窗候选 + 可复现打散”，与论文中的“效率优先重排”方向一致。
  - `backend/src/interfaces/api/RecommendationController.cpp`：
    - 共鸣/按情绪发现查询统一改为使用 `stones.ripple_count` 与 `deleted_at IS NULL`，减少子查询开销并清理脏数据干扰。
  - `backend/src/infrastructure/ai/DualMemoryRAG.cpp`：
    - 模板句检测从字符串直匹配升级为归一化匹配，拦截“同句不同标点/空白”的套话变体。

### 9. 2026-02-23 追加修复（不改架构）
- `backend/src/infrastructure/ai/AIService.cpp`
  - `generateReply` 在 RAG/带上下文场景关闭语义缓存复用，仅在无上下文问答场景启用缓存。
  - 目的：修复“不同问题返回同一条湖神回复”的误命中问题。
- `backend/src/interfaces/api/EdgeAIController.cpp`
  - `POST /api/lake-god/chat` 入参兼容 `content` 与 `message` 双字段。
  - 目的：兼容旧版前端与调试脚本，减少 400 参数错误。
- `backend/src/infrastructure/ai/EdgeAIEngine.cpp`
  - 增加失眠/焦虑/躯体化高相关词条（如：`失眠/睡不着/心慌/精疲力尽` 等）。
  - 中文短语匹配窗口从 `2~3` 扩展到 `2~6` 字，提升“短句+短语”识别率。
  - 否定词作用改为短窗口衰减，避免中文短语中的“`不`”跨句误翻转情绪极性。
  - ONNX 模型/词表路径增加运行时相对路径回退（当前目录、上级目录、`backend/` 目录），减少“启动目录不同导致找不到模型”问题。
- `backend/src/main.cpp`
  - 启动时自动加载 `.env`（支持 `.env`、`../.env`、`./backend/.env`，或 `HEARTLAKE_ENV_PATH` 指定）。
  - AI 默认值调整为本地优先：`provider=ollama`、`base_url=http://127.0.0.1:11434`、`model=heartlake-qwen`。
  - 目的：手动启动二进制时不再因漏 source 环境变量导致本地模型链路失效。
- `backend/models/sentiment_zh.onnx` / `backend/models/vocab.txt`
  - 替换为校验通过的 ONNX 量化模型资产：`onnx-community/Erlangshen-Roberta-110M-Sentiment-ONNX/onnx/model_int8.onnx` + `vocab.txt`。
  - 新模型哈希：`sha256=5e0e07161a093d7134f0a3293ae6c3c6e9bae2d73c6bfbba65ae26e3a512f466`。
  - 目的：修复启动日志中的 `Protobuf parsing failed`，恢复 ONNX 本地推理链路。

### 10. 2026-02-23 联调验收结果（手动启动，无 Docker）
- 启动日志确认：
  - `[OnnxSentiment] Engine initialized successfully`
  - `[EdgeAI] ONNX sentiment engine enabled: ./models/sentiment_zh.onnx`
- 核心接口实测（`Origin=http://127.0.0.1:5173`）：
  - `POST /api/auth/anonymous` -> `200`（携带 `device_id`）
  - `GET /api/lake/stones?page=1&page_size=20&sort=latest` -> 连续 10 次 `200`
  - `POST /api/lake-god/chat` -> `200`（`content`/`message` 双字段均可）
  - `POST /api/edge-ai/analyze` -> `method=onnx_ensemble`
  - 鉴权失败场景（如 `/api/friends`、`/api/stones/my`）返回 `401` 且带 `Access-Control-Allow-Origin`，前端不再出现假性 CORS 失败

### 11. ONNX Runtime GPU 路径统一（2026-02-23）
- `backend/cmake/FindOnnxRuntime.cmake`
  - 增加 CPU/GPU 双包探测，默认优先 `onnxruntime-linux-x64-gpu-1.22.0`。
  - 刷新 CMake 缓存变量，避免被旧的 CPU 路径缓存“锁死”。
- `backend/start.sh`
  - 启动时统一导出 `ONNXRUNTIME_ROOT` 与 `LD_LIBRARY_PATH`。
  - 自动注入 `third_party/onnxruntime-*` 与 `/usr/local/lib/ollama/*` 路径，减少运行时动态库缺失。
- 实机结果（当前机器）：
  - ONNX 已确认走 `onnx_ensemble` 链路；
  - 由于系统缺 `libcurand.so.10` / `libcufft.so.11` / `libnvrtc.so.12`，CUDA EP 自动回退 CPU（服务可用，日志可定位）。

### 12. 亲密分算法重构 + 漂流模式彻底下线（2026-02-23）
- `backend/src/infrastructure/services/IntimacyService.cpp`
  - 将亲密分升级为多信号融合模型（互动强度 + 双向互惠 + 共兴趣涟漪 + 情绪兼容 + 用户语义相似 + 情绪趋势对齐 + 时间衰减）。
  - 增加分项输出：`interaction_strength / reciprocity_score / co_ripple_score / mood_resonance / semantic_similarity / emotion_alignment / freshness`。
  - 引入低样本保护（互动次数过少时分数封顶），避免“单次偶遇”被误判为强关系。
- `backend/src/interfaces/api/FriendController.cpp`
  - 好友列表新增 `score_breakdown` 与 `ai_compatibility` 字段，保留原字段兼容旧前端。
  - 维持“无接受/拒绝”自动关系模式：`mode=intimacy_auto`。
- `backend/src/interfaces/api/PaperBoatController.cpp` / `backend/include/interfaces/api/PaperBoatController.h`
  - 删除漂流模式残留接口实现（`drift/catch/respond/release/status/count`），仅保留石头评论纸船链路。
- `frontend/lib/presentation/screens/profile_screen.dart`
  - 移除漂流瓶入口，避免无效 API 调用与误导交互。

### 13. 情绪判定准确度与界面信息架构修复（2026-02-23）
- `backend/src/infrastructure/ai/EdgeAIEngine.cpp`
  - 新增正向事件语义修正（如“收到礼物/被表扬/通过”等），在无负向上下文时避免误判为焦虑或中性。
  - ONNX 与规则融合冲突时加入“正向事件保护权重”，降低 ONNX 异常偏置影响。
- `frontend/lib/providers/edge_ai_provider.dart`
  - 远端失败降级路径从“伪随机特征分配”升级为“规则分类 + 本地模型融合”。
  - 增加上下文校正（礼物/夸奖等正向事件）并扩展情绪标签映射，减少前端展示偏差。
- `frontend/lib/presentation/screens/emotion_calendar_screen.dart`
  - 将“情绪日历”和“情绪热力图”拆分，不再混在同一页面。
- `frontend/lib/presentation/screens/emotion_heatmap_screen.dart`（新增）
  - 热力图独立展示，并保留情绪洞察卡片。
- `frontend/lib/presentation/screens/onboarding_screen.dart`
  - 纸船文案改为“石头下评论”语义，去除漂流模式描述。

## 近两年参考技术（联网检索）
- OpenAI（2024）结构化输出：提升 JSON 可解析性与一致性  
  https://openai.com/index/introducing-structured-outputs-in-the-api/
- ONNX Runtime Releases（2025）：持续优化 EP 选择与推理能力（v1.21~v1.22）  
  https://github.com/microsoft/onnxruntime/releases
- ONNX Runtime Quantization（官方文档，2024-2025 持续更新）：量化策略与精度/性能权衡  
  https://onnxruntime.ai/docs/performance/model-optimizations/quantization.html
- LongEmotion（2025）：长上下文情绪理解与历史依赖建模  
  https://arxiv.org/abs/2504.14823
- IntentionESC（2025）：意图识别驱动的情感支持回复策略  
  https://arxiv.org/abs/2503.02859
- AER-LLM（2024）：歧义情绪识别与上下文建模  
  https://arxiv.org/abs/2409.18339
- The Impacts of Data, Ordering, and Intrinsic Dimensionality on Recall in HNSW（2024）  
  https://arxiv.org/abs/2405.17813
- The "H" in HNSW Stands for "Hubs"（2024）  
  https://arxiv.org/abs/2410.01231
- Ada-EF: Dynamic Exploration for Approximate Nearest Neighbor Search（2025）  
  https://arxiv.org/abs/2512.06636
- Diversity-aware Search in High-dimensional Sparse Spaces（2024）  
  https://arxiv.org/abs/2402.14769

## 兼容性说明
- API 路径不变；新增字段为向后兼容扩展。
- 旧客户端可继续使用原字段；新客户端可使用 `normalized_score` 与 `trend`。
