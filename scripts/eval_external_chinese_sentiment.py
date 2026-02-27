#!/usr/bin/env python3
"""
Evaluate HeartLake /api/edge-ai/analyze on sepidmnorozy/Chinese_sentiment.

Requirements:
  pip install datasets aiohttp
"""

from __future__ import annotations

import argparse
import asyncio
import datetime as dt
import json
import math
import statistics
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

try:
    import aiohttp
except Exception as exc:  # pragma: no cover
    raise SystemExit("missing dependency: aiohttp (pip install aiohttp)") from exc

try:
    from datasets import load_dataset
except Exception as exc:  # pragma: no cover
    raise SystemExit("missing dependency: datasets (pip install datasets)") from exc

POS_MOODS = {"happy", "joy", "surprised", "surprise", "calm"}
NEG_MOODS = {"sad", "sadness", "anxious", "fear", "angry", "anger", "confused"}


def percentile(values: list[float], p: float) -> float:
    if not values:
        return 0.0
    values_sorted = sorted(values)
    idx = min(len(values_sorted) - 1, max(0, int(math.ceil(p * len(values_sorted))) - 1))
    return float(values_sorted[idx])


def compute_binary_metrics(rows: list[dict[str, Any]], key: str) -> dict[str, Any]:
    tp = tn = fp = fn = 0
    for row in rows:
        y = int(row["label"])
        p = int(row[key])
        if y == 1 and p == 1:
            tp += 1
        elif y == 0 and p == 0:
            tn += 1
        elif y == 0 and p == 1:
            fp += 1
        else:
            fn += 1

    total = tp + tn + fp + fn
    acc = (tp + tn) / total if total else 0.0
    precision = tp / (tp + fp) if (tp + fp) else 0.0
    recall = tp / (tp + fn) if (tp + fn) else 0.0
    f1 = (2 * precision * recall / (precision + recall)) if (precision + recall) else 0.0
    return {
        "accuracy": round(acc, 4),
        "precision": round(precision, 4),
        "recall": round(recall, 4),
        "f1": round(f1, 4),
        "confusion": {"tp": tp, "tn": tn, "fp": fp, "fn": fn},
    }


async def auth_anonymous(session: aiohttp.ClientSession, base_url: str, device_id: str) -> tuple[str, str]:
    async with session.post(f"{base_url}/api/auth/anonymous", json={"device_id": device_id}) as resp:
        body = await resp.json()
        if resp.status != 200:
            raise RuntimeError(f"auth failed: status={resp.status}, body={body}")
        data = body.get("data", {})
        token = str(data.get("token", ""))
        user_id = str(data.get("user_id", ""))
        if not token or not user_id:
            raise RuntimeError(f"auth missing token/user_id: {body}")
        return token, user_id


async def run_split(base_url: str, split: str, concurrency: int) -> dict[str, Any]:
    dataset = load_dataset("sepidmnorozy/Chinese_sentiment", split=split)
    queue: asyncio.Queue[tuple[int, str, int]] = asyncio.Queue()
    for i, item in enumerate(dataset):
        queue.put_nowait((i, str(item["text"]), int(item["label"])))

    timeout = aiohttp.ClientTimeout(total=30)
    async with aiohttp.ClientSession(timeout=timeout) as session:
        token, user_id = await auth_anonymous(
            session,
            base_url,
            f"external-{split}-{dt.datetime.now(dt.UTC).strftime('%Y%m%d%H%M%S')}",
        )
        headers = {
            "Authorization": f"Bearer {token}",
            "X-User-Id": user_id,
            "Content-Type": "application/json",
        }

        latencies: list[float] = []
        status = Counter()
        rows: list[dict[str, Any]] = []
        errors = 0

        async def worker() -> None:
            nonlocal errors
            while True:
                try:
                    sample_idx, text, label = queue.get_nowait()
                except asyncio.QueueEmpty:
                    return

                t0 = asyncio.get_running_loop().time()
                try:
                    async with session.post(
                        f"{base_url}/api/edge-ai/analyze",
                        headers=headers,
                        json={"text": text},
                    ) as resp:
                        dt_ms = (asyncio.get_running_loop().time() - t0) * 1000.0
                        latencies.append(dt_ms)
                        status[resp.status] += 1

                        if resp.status != 200:
                            errors += 1
                            continue

                        body = await resp.json()
                        data = body.get("data", {})
                        score = float(data.get("score", 0.0))
                        mood = str(data.get("mood", "neutral")).lower()
                        method = str(data.get("method", "")).strip().lower()
                        confidence_raw = data.get("confidence", 0.0)
                        try:
                            confidence = float(confidence_raw)
                        except Exception:
                            confidence = 0.0
                        pred_sign = 1 if score >= 0 else 0
                        if mood in POS_MOODS:
                            pred_hybrid = 1
                        elif mood in NEG_MOODS:
                            pred_hybrid = 0
                        else:
                            pred_hybrid = pred_sign

                        rows.append(
                            {
                                "index": int(sample_idx),
                                "label": label,
                                "pred_score_sign": pred_sign,
                                "pred_hybrid": pred_hybrid,
                                "score": score,
                                "mood": mood,
                                "method": method,
                                "confidence": confidence,
                            }
                        )
                except Exception:
                    errors += 1
                finally:
                    queue.task_done()

        t_start = asyncio.get_running_loop().time()
        workers = [asyncio.create_task(worker()) for _ in range(concurrency)]
        await asyncio.gather(*workers)
        elapsed = max(0.001, asyncio.get_running_loop().time() - t_start)

    method_total = Counter()
    method_correct = Counter()
    for row in rows:
        method = str(row.get("method", "")).strip() or "unknown"
        method_total[method] += 1
        if int(row["pred_score_sign"]) == int(row["label"]):
            method_correct[method] += 1

    method_breakdown: dict[str, Any] = {}
    for method in sorted(method_total.keys()):
        total = method_total[method]
        method_breakdown[method] = {
            "n": total,
            "accuracy": round(method_correct[method] / total, 4) if total else 0.0,
        }

    result = {
        "samples_total": len(dataset),
        "samples_scored": len(rows),
        "errors": errors,
        "status_counts": {str(k): int(v) for k, v in sorted(status.items())},
        "throughput_rps": round(len(rows) / elapsed, 2),
        "latency_ms": {
            "p50": round(percentile(latencies, 0.50), 2),
            "p95": round(percentile(latencies, 0.95), 2),
            "p99": round(percentile(latencies, 0.99), 2),
            "avg": round(statistics.mean(latencies), 2) if latencies else 0.0,
        },
        "metrics_score_sign": compute_binary_metrics(rows, "pred_score_sign"),
        "metrics_hybrid_mood_score": compute_binary_metrics(rows, "pred_hybrid"),
        "method_breakdown": method_breakdown,
        "rows": rows,
    }
    return result


def to_md(report: dict[str, Any]) -> str:
    lines = ["# 外部数据集评测（Chinese_sentiment）", ""]
    for split_name, item in report["splits"].items():
        m = item["metrics_score_sign"]
        lat = item["latency_ms"]
        lines.extend(
            [
                f"## {split_name}",
                f"- 样本: {item['samples_scored']}/{item['samples_total']}  errors={item['errors']}",
                f"- accuracy={m['accuracy']} precision={m['precision']} recall={m['recall']} f1={m['f1']}",
                f"- latency p50={lat['p50']}ms p95={lat['p95']}ms p99={lat['p99']}ms avg={lat['avg']}ms",
                f"- throughput={item['throughput_rps']} req/s",
                "",
            ]
        )
    return "\n".join(lines).rstrip() + "\n"


async def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-url", default="http://127.0.0.1:8080")
    parser.add_argument("--splits", default="validation,test", help="comma-separated: validation,test")
    parser.add_argument("--concurrency", type=int, default=24)
    parser.add_argument(
        "--out-json",
        default="docs/外部数据集评测_Chinese_sentiment_2026-02-26.json",
    )
    parser.add_argument(
        "--out-md",
        default="docs/外部数据集评测_Chinese_sentiment_2026-02-26.md",
    )
    parser.add_argument(
        "--out-rows",
        default="",
        help="optional: write per-sample prediction rows to this JSON file",
    )
    args = parser.parse_args()

    split_names = [x.strip() for x in args.splits.split(",") if x.strip()]
    for s in split_names:
        if s not in {"validation", "test"}:
            raise SystemExit(f"unsupported split: {s}")

    splits: dict[str, Any] = {}
    split_rows: dict[str, Any] = defaultdict(list)
    for s in split_names:
        alias = "dev" if s == "validation" else s
        result = await run_split(args.base_url, s, args.concurrency)
        split_rows[alias] = result.pop("rows", [])
        splits[alias] = result

    report = {
        "dataset": "sepidmnorozy/Chinese_sentiment",
        "timestamp": dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "splits": splits,
    }

    out_json = Path(args.out_json)
    out_md = Path(args.out_md)
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_md.parent.mkdir(parents=True, exist_ok=True)

    out_json.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    out_md.write_text(to_md(report), encoding="utf-8")

    if args.out_rows:
        out_rows = Path(args.out_rows)
        out_rows.parent.mkdir(parents=True, exist_ok=True)
        rows_report = {
            "dataset": report["dataset"],
            "timestamp": report["timestamp"],
            "splits": split_rows,
        }
        out_rows.write_text(json.dumps(rows_report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    asyncio.run(main())
