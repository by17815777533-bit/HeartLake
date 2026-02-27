#!/usr/bin/env python3
"""
Paired significance test (McNemar) for two prediction row dumps.

Input row files should be created by scripts/eval_external_chinese_sentiment.py --out-rows.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any


def load_rows(path: Path, split: str) -> list[dict[str, Any]]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    rows = payload.get("splits", {}).get(split, [])
    if not isinstance(rows, list):
        raise RuntimeError(f"split '{split}' not found in {path}")
    return rows


def binom_two_sided_p_value(n: int, b: int, c: int) -> float:
    if n <= 0:
        return 1.0
    k = min(b, c)
    tail = 0.0
    denom = 2.0 ** n
    for i in range(0, k + 1):
        tail += math.comb(n, i) / denom
    return min(1.0, 2.0 * tail)


def chi_square_cc_p_value(b: int, c: int) -> tuple[float, float]:
    n = b + c
    if n <= 0:
        return 0.0, 1.0
    chi2 = (abs(b - c) - 1.0) ** 2 / n
    # df=1 survival function without scipy: p = erfc(sqrt(chi2 / 2)).
    p = math.erfc(math.sqrt(max(0.0, chi2) / 2.0))
    return chi2, p


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", required=True, help="rows json for baseline")
    parser.add_argument("--candidate", required=True, help="rows json for candidate")
    parser.add_argument("--split", default="dev", help="split name, default: dev")
    parser.add_argument("--key", default="pred_score_sign", help="prediction field in rows")
    args = parser.parse_args()

    baseline_rows = load_rows(Path(args.baseline), args.split)
    candidate_rows = load_rows(Path(args.candidate), args.split)

    baseline_map = {
        int(row["index"]): row
        for row in baseline_rows
        if "index" in row and "label" in row and args.key in row
    }
    candidate_map = {
        int(row["index"]): row
        for row in candidate_rows
        if "index" in row and "label" in row and args.key in row
    }
    common_indexes = sorted(set(baseline_map.keys()) & set(candidate_map.keys()))
    if not common_indexes:
        raise RuntimeError("no overlapping sample indexes between baseline and candidate")

    old_correct_new_correct = 0
    old_wrong_new_wrong = 0
    old_wrong_new_correct = 0  # b in McNemar
    old_correct_new_wrong = 0  # c in McNemar

    baseline_correct = 0
    candidate_correct = 0

    for idx in common_indexes:
        old = baseline_map[idx]
        new = candidate_map[idx]
        label = int(old["label"])
        old_pred = int(old[args.key])
        new_pred = int(new[args.key])

        old_ok = old_pred == label
        new_ok = new_pred == label
        baseline_correct += int(old_ok)
        candidate_correct += int(new_ok)

        if old_ok and new_ok:
            old_correct_new_correct += 1
        elif (not old_ok) and (not new_ok):
            old_wrong_new_wrong += 1
        elif (not old_ok) and new_ok:
            old_wrong_new_correct += 1
        else:
            old_correct_new_wrong += 1

    n = len(common_indexes)
    b = old_wrong_new_correct
    c = old_correct_new_wrong
    exact_p = binom_two_sided_p_value(b + c, b, c)
    chi2_cc, approx_p = chi_square_cc_p_value(b, c)

    report = {
        "paired_samples": n,
        "prediction_key": args.key,
        "baseline_accuracy": round(baseline_correct / n, 6),
        "candidate_accuracy": round(candidate_correct / n, 6),
        "delta_accuracy_pp": round((candidate_correct - baseline_correct) * 100.0 / n, 4),
        "mcnemar_table": {
            "old_correct_new_correct": old_correct_new_correct,
            "old_wrong_new_wrong": old_wrong_new_wrong,
            "old_wrong_new_correct": b,
            "old_correct_new_wrong": c,
        },
        "mcnemar": {
            "exact_two_sided_p": exact_p,
            "chi_square_cc": chi2_cc,
            "chi_square_cc_p": approx_p,
        },
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
