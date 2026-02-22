#!/usr/bin/env python3
"""
HeartLake 情感分析模型导出脚本

将 Erlangshen-Roberta-110M-Sentiment 导出为 ONNX 格式并进行 INT8 量化。

使用方法:
    pip install transformers torch onnx onnxruntime optimum
    python scripts/export_onnx.py

输出:
    backend/models/sentiment_zh.onnx      - 量化后的 ONNX 模型
    backend/models/vocab.txt              - BERT 词表
"""

import os
import shutil
import torch
from pathlib import Path
from transformers import AutoTokenizer, AutoModelForSequenceClassification

MODEL_NAME = "IDEA-CCNL/Erlangshen-Roberta-110M-Sentiment"
OUTPUT_DIR = Path(__file__).parent.parent / "backend" / "models"
ONNX_PATH = OUTPUT_DIR / "sentiment_zh.onnx"
ONNX_QUANTIZED_PATH = OUTPUT_DIR / "sentiment_zh_int8.onnx"
VOCAB_PATH = OUTPUT_DIR / "vocab.txt"
MAX_SEQ_LEN = 128


def export_onnx():
    print(f"[1/4] 下载模型: {MODEL_NAME}")
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
    model = AutoModelForSequenceClassification.from_pretrained(MODEL_NAME)
    model.eval()

    # 创建输出目录
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # 复制 vocab.txt
    print("[2/4] 复制词表文件")
    vocab_source = Path(tokenizer.vocab_file)
    shutil.copy2(vocab_source, VOCAB_PATH)
    print(f"  词表已保存: {VOCAB_PATH} ({os.path.getsize(VOCAB_PATH) / 1024:.1f} KB)")

    # 构造 dummy input
    print("[3/4] 导出 ONNX 模型")
    dummy_text = "今天心情很好"
    inputs = tokenizer(
        dummy_text,
        return_tensors="pt",
        max_length=MAX_SEQ_LEN,
        padding="max_length",
        truncation=True,
    )

    input_ids = inputs["input_ids"]
    attention_mask = inputs["attention_mask"]
    token_type_ids = inputs["token_type_ids"]

    # 导出 ONNX
    torch.onnx.export(
        model,
        (input_ids, attention_mask, token_type_ids),
        str(ONNX_PATH),
        input_names=["input_ids", "attention_mask", "token_type_ids"],
        output_names=["logits"],
        dynamic_axes={
            "input_ids": {0: "batch_size", 1: "sequence_length"},
            "attention_mask": {0: "batch_size", 1: "sequence_length"},
            "token_type_ids": {0: "batch_size", 1: "sequence_length"},
            "logits": {0: "batch_size"},
        },
        opset_version=14,
        do_constant_folding=True,
    )
    fp32_size = os.path.getsize(ONNX_PATH) / (1024 * 1024)
    print(f"  FP32 模型已保存: {ONNX_PATH} ({fp32_size:.1f} MB)")

    # INT8 动态量化
    print("[4/4] INT8 量化")
    try:
        from onnxruntime.quantization import quantize_dynamic, QuantType

        quantize_dynamic(
            str(ONNX_PATH),
            str(ONNX_QUANTIZED_PATH),
            weight_type=QuantType.QInt8,
        )
        int8_size = os.path.getsize(ONNX_QUANTIZED_PATH) / (1024 * 1024)
        print(f"  INT8 模型已保存: {ONNX_QUANTIZED_PATH} ({int8_size:.1f} MB)")
        print(f"  压缩比: {fp32_size / int8_size:.1f}x")

        # 用量化模型替换原模型
        os.replace(str(ONNX_QUANTIZED_PATH), str(ONNX_PATH))
        print(f"  已用量化模型替换原模型")
    except ImportError:
        print("  [跳过] onnxruntime.quantization 未安装，使用 FP32 模型")

    # 验证
    print("\n验证模型...")
    import onnxruntime as ort
    import numpy as np

    sess = ort.InferenceSession(str(ONNX_PATH))
    test_texts = [
        ("今天心情很好，阳光明媚", "positive"),
        ("难过得想哭，什么都不想做", "negative"),
        ("虽然下雨了但心情不错", "positive"),
        ("不开心", "negative"),
        ("还行吧，一般般", "neutral-ish"),
    ]

    print(f"{'文本':<25} {'预期':<12} {'neg_prob':<10} {'pos_prob':<10} {'判定'}")
    print("-" * 70)
    for text, expected in test_texts:
        enc = tokenizer(text, return_tensors="np", max_length=MAX_SEQ_LEN,
                        padding="max_length", truncation=True)
        logits = sess.run(None, {
            "input_ids": enc["input_ids"].astype(np.int64),
            "attention_mask": enc["attention_mask"].astype(np.int64),
            "token_type_ids": enc["token_type_ids"].astype(np.int64),
        })[0]
        probs = np.exp(logits) / np.exp(logits).sum(axis=-1, keepdims=True)
        neg_prob, pos_prob = probs[0]
        label = "positive" if pos_prob > neg_prob else "negative"
        print(f"{text:<25} {expected:<12} {neg_prob:.4f}     {pos_prob:.4f}     {label}")

    print(f"\n完成! 模型文件: {ONNX_PATH}")
    print(f"词表文件: {VOCAB_PATH}")


if __name__ == "__main__":
    export_onnx()
