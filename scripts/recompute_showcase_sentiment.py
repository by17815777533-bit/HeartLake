#!/usr/bin/env python3
"""为 showcase 石头执行一轮真实情感判断，并回写 mood_type / emotion_score / sentiment_score。"""

from __future__ import annotations

import json
import os
import random
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Dict, List, Tuple


REPO_ROOT = Path(__file__).resolve().parents[1]
ENV_PATH = REPO_ROOT / "backend" / ".env"


def read_env(env_path: Path) -> Dict[str, str]:
    env: Dict[str, str] = {}
    for raw in env_path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        env[key.strip()] = value.strip()
    return env


def run_psql_query(config: Dict[str, str], sql: str) -> str:
    env = os.environ.copy()
    env["PGPASSWORD"] = config["DB_PASSWORD"]
    cmd = [
        "psql",
        "-h",
        config["DB_HOST"],
        "-p",
        config["DB_PORT"],
        "-U",
        config["DB_USER"],
        "-d",
        config["DB_NAME"],
        "-At",
        "-F",
        "\t",
        "-v",
        "ON_ERROR_STOP=1",
        "-c",
        sql,
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, env=env, check=True)
    return proc.stdout


def run_psql_script(config: Dict[str, str], sql: str) -> None:
    env = os.environ.copy()
    env["PGPASSWORD"] = config["DB_PASSWORD"]
    cmd = [
        "psql",
        "-h",
        config["DB_HOST"],
        "-p",
        config["DB_PORT"],
        "-U",
        config["DB_USER"],
        "-d",
        config["DB_NAME"],
        "-v",
        "ON_ERROR_STOP=1",
    ]
    subprocess.run(cmd, input=sql, text=True, env=env, check=True)


def post_json(url: str, payload: Dict[str, object], headers: Dict[str, str]) -> Dict[str, object]:
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json", **headers},
    )
    with urllib.request.urlopen(req, timeout=15) as resp:
        return json.loads(resp.read().decode("utf-8"))


def normalize_mood(raw_mood: str) -> str:
    mood = raw_mood.strip().lower()
    mapping = {
        "joy": "happy",
        "happy": "happy",
        "sadness": "sad",
        "sad": "sad",
        "anger": "angry",
        "angry": "angry",
        "fear": "anxious",
        "anxious": "anxious",
        "anxiety": "anxious",
        "surprise": "surprised",
        "surprised": "surprised",
        "calm": "calm",
        "peaceful": "peaceful",
        "neutral": "neutral",
        "grateful": "grateful",
        "gratitude": "grateful",
        "hopeful": "hopeful",
        "lonely": "lonely",
        "confused": "confused",
    }
    return mapping.get(mood, "neutral")


def sql_quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def main() -> int:
    if not ENV_PATH.exists():
        print(f"missing env file: {ENV_PATH}", file=sys.stderr)
        return 1

    config = read_env(ENV_PATH)
    needed = ["DB_HOST", "DB_PORT", "DB_NAME", "DB_USER", "DB_PASSWORD"]
    missing = [k for k in needed if not config.get(k)]
    if missing:
        print(f"missing db config in .env: {', '.join(missing)}", file=sys.stderr)
        return 1

    api_base = config.get("PUBLIC_API_URL", "http://127.0.0.1:8080").rstrip("/")
    auth_url = f"{api_base}/api/auth/anonymous"
    analyze_url = f"{api_base}/api/edge-ai/analyze"

    # ✍🏻 只处理可重复识别的展示石头，不动真实用户历史数据。
    rows_raw = run_psql_query(
        config,
        """
        SELECT stone_id, content
        FROM stones
        WHERE stone_id LIKE 'showcase_stone_%'
          AND status = 'published'
          AND deleted_at IS NULL
        ORDER BY created_at DESC;
        """,
    )
    rows: List[Tuple[str, str]] = []
    for line in rows_raw.splitlines():
        if not line.strip():
            continue
        stone_id, content = line.split("\t", 1)
        rows.append((stone_id, content))

    if not rows:
        print("no showcase stones found")
        return 0

    auth_resp = post_json(
        auth_url,
        {"device_id": f"seed-pass-{random.randint(100000, 999999)}"},
        headers={},
    )
    token = str((auth_resp.get("data") or {}).get("token") or "")
    if not token:
        print("failed to get anonymous token", file=sys.stderr)
        return 1

    updates: List[Tuple[str, str, float]] = []
    failed: List[str] = []
    mood_count: Dict[str, int] = {}

    # ✍🏻 按文本逐条走 /edge-ai/analyze，确保情绪标签来自真实推理而非硬编码。
    for idx, (stone_id, content) in enumerate(rows, start=1):
        text = content.strip()
        if text.startswith("showcase:"):
            text = text.split(":", 1)[1].strip()
        if not text:
            updates.append((stone_id, "neutral", 0.0))
            continue

        try:
            resp = post_json(
                analyze_url,
                {"text": text},
                headers={"Authorization": f"Bearer {token}"},
            )
            data = (resp.get("data") or {}) if isinstance(resp, dict) else {}
            raw_mood = str(data.get("mood") or data.get("sentiment") or "neutral")
            score = data.get("score")
            if not isinstance(score, (int, float)):
                score = 0.0
            score_f = max(-1.0, min(1.0, float(score)))
            mood = normalize_mood(raw_mood)
            updates.append((stone_id, mood, score_f))
            mood_count[mood] = mood_count.get(mood, 0) + 1
        except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError):
            failed.append(stone_id)
            updates.append((stone_id, "neutral", 0.0))

        if idx % 200 == 0:
            print(f"processed {idx}/{len(rows)}")
            time.sleep(0.05)

    if not updates:
        print("no updates produced", file=sys.stderr)
        return 1

    values = ",\n".join(
        f"({sql_quote(stone_id)}, {sql_quote(mood)}, {score:.6f})"
        for stone_id, mood, score in updates
    )
    sql = f"""
BEGIN;
CREATE TEMP TABLE tmp_showcase_sentiment_updates (
    stone_id TEXT PRIMARY KEY,
    mood_type TEXT NOT NULL,
    score DOUBLE PRECISION NOT NULL
);
INSERT INTO tmp_showcase_sentiment_updates (stone_id, mood_type, score)
VALUES
{values};

UPDATE stones s
SET mood_type = u.mood_type,
    emotion_score = u.score,
    sentiment_score = u.score,
    updated_at = NOW()
FROM tmp_showcase_sentiment_updates u
WHERE s.stone_id = u.stone_id;
COMMIT;
"""
    run_psql_script(config, sql)

    print(f"updated showcase stones: {len(updates)}")
    print("mood distribution:", json.dumps(mood_count, ensure_ascii=False))
    if failed:
        print(f"analyze failed fallback count: {len(failed)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
