# HeartLake ONNX 模型目录

该目录默认不把大模型文件提交到 Git。

恢复命令：

```bash
./scripts/restore-onnx-model.sh
```

恢复后将生成：

- `backend/models/sentiment_zh.onnx`
- `backend/models/vocab.txt`
- `backend/models/sentiment_domain_lexicon.tsv`

默认下载源：

- ONNX：`onnx-community/Erlangshen-Roberta-110M-Sentiment-ONNX` 的 `model_int8.onnx`
- 词表：`IDEA-CCNL/Erlangshen-Roberta-110M-Sentiment` 的 `vocab.txt`
