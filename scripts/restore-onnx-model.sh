#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODELS_DIR="${ROOT_DIR}/backend/models"
mkdir -p "${MODELS_DIR}"

MODEL_URL="${MODEL_URL:-https://huggingface.co/onnx-community/Erlangshen-Roberta-110M-Sentiment-ONNX/resolve/main/onnx/model_int8.onnx?download=true}"
VOCAB_URL="${VOCAB_URL:-https://huggingface.co/IDEA-CCNL/Erlangshen-Roberta-110M-Sentiment/resolve/main/vocab.txt?download=true}"
MODEL_SHA256="${MODEL_SHA256:-5e0e07161a093d7134f0a3293ae6c3c6e9bae2d73c6bfbba65ae26e3a512f466}"

MODEL_PATH="${MODELS_DIR}/sentiment_zh.onnx"
VOCAB_PATH="${MODELS_DIR}/vocab.txt"

download_file() {
  local url="$1"
  local output="$2"
  local tmp="${output}.part"
  curl --fail --location --retry 3 --retry-delay 2 --output "${tmp}" "${url}"
  mv "${tmp}" "${output}"
}

echo "[onnx] 下载 INT8 中文情绪分类模型到 ${MODEL_PATH}"
download_file "${MODEL_URL}" "${MODEL_PATH}"

echo "[onnx] 校验模型 SHA256"
echo "${MODEL_SHA256}  ${MODEL_PATH}" | sha256sum --check --status

echo "[onnx] 下载 vocab 到 ${VOCAB_PATH}"
download_file "${VOCAB_URL}" "${VOCAB_PATH}"

echo "[onnx] 已恢复模型文件"
echo "[onnx] 模型: ${MODEL_PATH}"
echo "[onnx] 词表: ${VOCAB_PATH}"
