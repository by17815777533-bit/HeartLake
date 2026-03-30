# 数据模型说明

`backend/models/` 当前只维护仍在运行时和迁移中使用的模型资源说明。

## 当前用途

- 运行期模型资源存放
- ONNX 情绪分析模型加载
- 词表和领域情绪词典加载
- 代码与迁移的资源来源说明

## 当前目录内容

- `sentiment_zh.onnx`
- `vocab.txt`
- `sentiment_domain_lexicon.tsv`

## 当前资源用途

### `sentiment_zh.onnx`

- 当前由 `backend/src/infrastructure/ai/OnnxSentimentEngine.cpp` 使用
- 为 Edge AI 和情绪分析链提供本地 ONNX 推理能力

### `vocab.txt`

- 当前由情绪分析和分词相关初始化路径加载
- 为 ONNX 模型推理提供词表映射

### `sentiment_domain_lexicon.tsv`

- 当前为情绪分析和领域词典补充提供数据
- 用于增强中文情绪词识别

## 当前约束

- 这个目录不放 ORM 实体类，ORM 对照以迁移和代码为准
- 模型资源路径必须和环境变量或默认加载路径一致
- 替换模型文件前必须核对 `EdgeAIEngine`、`OnnxSentimentEngine`、`SentimentAnalyzer` 的加载逻辑
- 新增模型资源时，需要同步更新部署手册和环境变量说明
