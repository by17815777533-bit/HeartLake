#!/usr/bin/env python3
"""
HeartLake 评审级验证脚本（功能 + 安全 + 一致性 + 精度 + 压力 + 门槛判定）

用法:
  python3 scripts/worldclass_validation.py --base-url http://127.0.0.1:8080
"""

from __future__ import annotations

import argparse
import asyncio
import json
import math
import random
import socket
import statistics
import time
import uuid
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Any, Dict, Iterable, List, Tuple
from urllib.parse import urlparse

import httpx
import requests


POSITIVE_MOODS = {"happy", "calm", "surprised"}
NEGATIVE_MOODS = {"anxious", "sad", "angry", "confused"}
NEUTRAL_MOODS = {"neutral"}
ALL_MOODS = ("happy", "calm", "anxious", "sad", "angry", "surprised", "confused", "neutral")

GATE_TARGETS = {
    "functional_pass_rate": 0.99,
    "security_pass_rate": 1.00,
    "consistency_pass_rate": 1.00,
    "mood_accuracy": 0.70,
    "macro_f1": 0.68,
    "polarity_accuracy": 0.90,
    "critical_case_hit_rate": 1.00,
    "stress_error_rate_max": 0.01,
    "stress_p95_ms_max": 800.0,
    "quic_udp_send_ok": True,
}

HTTPX_TRUST_ENV = True
REQUESTS_SESSION = requests.Session()


@dataclass
class AuthCtx:
    token: str
    user_id: str
    nickname: str


def configure_http_clients(base_url: str) -> None:
    global HTTPX_TRUST_ENV
    parsed = urlparse(base_url)
    host = (parsed.hostname or "").strip().lower()
    if host in {"127.0.0.1", "localhost", "::1"}:
        # 本地压测默认绕过系统代理，避免代理吞吐成为误差来源。
        HTTPX_TRUST_ENV = False
        REQUESTS_SESSION.trust_env = False
        REQUESTS_SESSION.proxies.clear()
    else:
        HTTPX_TRUST_ENV = True
        REQUESTS_SESSION.trust_env = True


def classify_polarity(score: float, mood: str) -> str:
    if score >= 0.2 or mood in POSITIVE_MOODS:
        return "positive"
    if score <= -0.2 or mood in NEGATIVE_MOODS:
        return "negative"
    return "neutral"


def must_json(resp: requests.Response) -> Dict[str, Any]:
    try:
        body = resp.json()
    except Exception as exc:
        raise RuntimeError(f"invalid json: status={resp.status_code}, body={resp.text[:240]}") from exc
    if not isinstance(body, dict):
        raise RuntimeError(f"invalid json shape: status={resp.status_code}, body={resp.text[:240]}")
    return body


def assert_ok(resp: requests.Response, path: str) -> Dict[str, Any]:
    if resp.status_code not in (200, 201):
        raise RuntimeError(f"{path} status={resp.status_code}, body={resp.text[:240]}")
    data = must_json(resp)
    if "code" not in data:
        raise RuntimeError(f"{path} invalid response shape: {resp.text[:240]}")
    return data


def auth_anonymous(base_url: str, device_tag: str) -> AuthCtx:
    payload = {"device_id": f"{device_tag}-{uuid.uuid4().hex[:10]}"}
    resp = REQUESTS_SESSION.post(f"{base_url}/api/auth/anonymous", json=payload, timeout=20)
    data = assert_ok(resp, "/api/auth/anonymous")
    token = str(data.get("data", {}).get("token", ""))
    user_id = str(data.get("data", {}).get("user_id", ""))
    nickname = str(data.get("data", {}).get("nickname", ""))
    if not token or not user_id:
        raise RuntimeError(f"anonymous auth missing token/user_id: {data}")
    return AuthCtx(token=token, user_id=user_id, nickname=nickname)


def headers(ctx: AuthCtx) -> Dict[str, str]:
    return {
        "Authorization": f"Bearer {ctx.token}",
        "X-User-Id": ctx.user_id,
        "Content-Type": "application/json",
    }


def parse_levels(raw: str) -> List[int]:
    levels: List[int] = []
    for part in raw.split(","):
        part = part.strip()
        if not part:
            continue
        try:
            val = int(part)
        except ValueError as exc:
            raise RuntimeError(f"invalid stress level: {part}") from exc
        if val <= 0:
            raise RuntimeError(f"stress level must be >0: {part}")
        levels.append(val)
    if not levels:
        raise RuntimeError("stress levels empty")
    return sorted(set(levels))


def extract_stone_id(body: Dict[str, Any]) -> str:
    data = body.get("data", {})
    if isinstance(data, dict):
        sid = data.get("stone_id")
        if isinstance(sid, str) and sid:
            return sid
        stone = data.get("stone")
        if isinstance(stone, dict):
            sid = stone.get("stone_id")
            if isinstance(sid, str) and sid:
                return sid
    return ""


def extract_detail(body: Dict[str, Any]) -> Dict[str, Any]:
    data = body.get("data", {})
    if not isinstance(data, dict):
        return {}
    stone = data.get("stone")
    if isinstance(stone, dict):
        return stone
    return data


def to_int(value: Any, default: int = 0) -> int:
    try:
        return int(value)
    except Exception:
        return default


def functional_suite(base_url: str, u1: AuthCtx, u2: AuthCtx) -> Dict[str, Any]:
    report: Dict[str, Any] = {"checks": [], "failed": []}
    now = datetime.now(timezone.utc)

    def run_check(
        method: str,
        path: str,
        ctx: AuthCtx | None = None,
        json_body: Any = None,
        expected: Tuple[int, ...] = (200, 201),
    ) -> Dict[str, Any]:
        url = f"{base_url}{path}"
        h = headers(ctx) if ctx else {"Content-Type": "application/json"}
        resp = REQUESTS_SESSION.request(method, url, headers=h, json=json_body, timeout=25)
        ok = resp.status_code in expected
        item = {"method": method, "path": path, "status": resp.status_code, "ok": ok}
        report["checks"].append(item)
        if not ok:
            report["failed"].append({**item, "body": resp.text[:320]})
            return {}
        try:
            return resp.json()
        except Exception:
            return {}

    # 1) 公开接口
    run_check("GET", "/api/health")
    run_check("GET", "/api/lake/weather")
    run_check("GET", "/api/lake/stones?page=1&page_size=20&sort=latest")

    # 2) 石头链路
    stone_data = run_check(
        "POST",
        "/api/stones",
        u1,
        {
            "content": "今天被老师夸了，我很开心，也想把这份心情传给大家。",
            "mood_type": "happy",
            "stone_type": "medium",
            "stone_color": "#7A92A3",
            "is_anonymous": True,
        },
    )
    stone_id = extract_stone_id(stone_data)
    if not stone_id:
        raise RuntimeError("createStone success but missing stone_id")

    run_check("GET", f"/api/stones/{stone_id}")
    run_check("GET", "/api/stones/my?page=1&page_size=50", u1)
    run_check("POST", f"/api/stones/{stone_id}/ripples", u2, {})
    run_check(
        "POST",
        f"/api/stones/{stone_id}/boats",
        u2,
        {"content": "看到你的石头，愿你每天都被温柔对待。"},
    )
    run_check("GET", f"/api/stones/{stone_id}/boats?page=1&page_size=20", u1)
    run_check(
        "POST",
        "/api/boats/reply",
        u2,
        {"stone_id": stone_id, "content": "纸船来了，愿你继续发光。", "mood": "calm"},
    )
    run_check("GET", "/api/interactions/my/ripples?page=1&page_size=20", u2)
    run_check("GET", "/api/interactions/my/boats?page=1&page_size=20", u2)
    run_check("GET", "/api/boats/sent?page=1&page_size=20", u2)
    run_check("GET", "/api/boats/received?page=1&page_size=20", u1)

    # 3) 侧边业务
    run_check("GET", "/api/friends", u1)
    run_check("GET", "/api/recommendations/trending?limit=20", u1)
    run_check("GET", "/api/recommendations/emotion-trends", u1)
    run_check("GET", "/api/vip/status", u1)
    run_check("GET", "/api/notifications/unread-count", u1)
    run_check("GET", "/api/guardian/stats", u1)
    run_check("GET", "/api/guardian/insights", u1)
    run_check("GET", f"/api/users/my/emotion-calendar?year={now.year}&month={now.month}", u1)
    run_check("GET", "/api/users/my/emotion-heatmap", u1)

    # 4) EdgeAI
    run_check("GET", "/api/edge-ai/status", u1)
    run_check("GET", "/api/edge-ai/metrics", u1)
    run_check("GET", "/api/edge-ai/emotion-pulse", u1)
    run_check("GET", "/api/edge-ai/privacy-budget", u1)
    run_check("POST", "/api/edge-ai/emotion-sample", u1, {"score": 0.91, "mood": "happy"})
    run_check("POST", "/api/edge-ai/analyze", u1, {"text": "今天收到了礼物，真的非常开心。"})

    # 5) CORS
    cors_resp = REQUESTS_SESSION.options(
        f"{base_url}/api/friends",
        headers={
            "Origin": "http://127.0.0.1:5173",
            "Access-Control-Request-Method": "GET",
            "Access-Control-Request-Headers": "authorization,x-user-id,content-type",
        },
        timeout=20,
    )
    cors_ok = cors_resp.status_code in (200, 204) and "Access-Control-Allow-Origin" in cors_resp.headers
    report["checks"].append(
        {"method": "OPTIONS", "path": "/api/friends", "status": cors_resp.status_code, "ok": cors_ok}
    )
    if not cors_ok:
        report["failed"].append(
            {
                "method": "OPTIONS",
                "path": "/api/friends",
                "status": cors_resp.status_code,
                "headers": dict(cors_resp.headers),
            }
        )

    report["total_checks"] = len(report["checks"])
    report["passed_checks"] = sum(1 for c in report["checks"] if c["ok"])
    report["pass_rate"] = round(report["passed_checks"] / max(1, report["total_checks"]), 4)
    return report


def security_suite(base_url: str, u1: AuthCtx) -> Dict[str, Any]:
    report: Dict[str, Any] = {"checks": [], "failed": []}

    def add_check(name: str, ok: bool, detail: Dict[str, Any]) -> None:
        item = {"name": name, "ok": ok, **detail}
        report["checks"].append(item)
        if not ok:
            report["failed"].append(item)

    unauth_cases = [
        ("GET", "/api/friends"),
        ("GET", "/api/stones/my?page=1&page_size=20"),
        ("GET", "/api/vip/status"),
        ("GET", "/api/notifications/unread-count"),
    ]
    for method, path in unauth_cases:
        resp = REQUESTS_SESSION.request(method, f"{base_url}{path}", timeout=20)
        ok = resp.status_code in (401, 403)
        add_check("unauthorized_guard", ok, {"method": method, "path": path, "status": resp.status_code})

    bad_payload_resp = REQUESTS_SESSION.post(
        f"{base_url}/api/stones",
        headers=headers(u1),
        json={"content": "", "mood_type": "happy"},
        timeout=20,
    )
    add_check(
        "invalid_payload_not_500",
        bad_payload_resp.status_code in (400, 401, 403, 422),
        {"path": "/api/stones", "status": bad_payload_resp.status_code, "body": bad_payload_resp.text[:220]},
    )

    cors_block_resp = REQUESTS_SESSION.options(
        f"{base_url}/api/friends",
        headers={
            "Origin": "http://malicious.example.com",
            "Access-Control-Request-Method": "GET",
            "Access-Control-Request-Headers": "authorization,x-user-id,content-type",
        },
        timeout=20,
    )
    allow_origin = cors_block_resp.headers.get("Access-Control-Allow-Origin", "")
    cors_ok = allow_origin != "*"  # 避免全开放
    add_check(
        "cors_not_wildcard",
        cors_ok,
        {"path": "/api/friends", "status": cors_block_resp.status_code, "allow_origin": allow_origin},
    )

    report["total_checks"] = len(report["checks"])
    report["passed_checks"] = sum(1 for c in report["checks"] if c["ok"])
    report["pass_rate"] = round(report["passed_checks"] / max(1, report["total_checks"]), 4)
    return report


def consistency_suite(base_url: str, u1: AuthCtx, u2: AuthCtx) -> Dict[str, Any]:
    report: Dict[str, Any] = {"checks": [], "failed": []}

    def add_check(name: str, ok: bool, detail: Dict[str, Any]) -> None:
        item = {"name": name, "ok": ok, **detail}
        report["checks"].append(item)
        if not ok:
            report["failed"].append(item)

    create_resp = REQUESTS_SESSION.post(
        f"{base_url}/api/stones",
        headers=headers(u1),
        json={
            "content": "一致性测试：记录计数变化。",
            "mood_type": "calm",
            "stone_type": "medium",
            "stone_color": "#89A7B5",
            "is_anonymous": True,
        },
        timeout=20,
    )
    create_body = assert_ok(create_resp, "/api/stones")
    stone_id = extract_stone_id(create_body)
    if not stone_id:
        raise RuntimeError("consistency create stone missing id")

    detail_before_resp = REQUESTS_SESSION.get(f"{base_url}/api/stones/{stone_id}", headers=headers(u1), timeout=20)
    detail_before = assert_ok(detail_before_resp, f"/api/stones/{stone_id}")
    before = extract_detail(detail_before)
    boat_before = to_int(before.get("boat_count", 0), 0)
    ripple_before = to_int(before.get("ripple_count", 0), 0)

    ripple_resp = REQUESTS_SESSION.post(
        f"{base_url}/api/stones/{stone_id}/ripples",
        headers=headers(u2),
        json={},
        timeout=20,
    )
    add_check(
        "create_ripple",
        ripple_resp.status_code in (200, 201),
        {"status": ripple_resp.status_code, "body": ripple_resp.text[:180]},
    )

    boat_resp = REQUESTS_SESSION.post(
        f"{base_url}/api/stones/{stone_id}/boats",
        headers=headers(u2),
        json={"content": "一致性校验纸船"},
        timeout=20,
    )
    add_check(
        "create_boat",
        boat_resp.status_code in (200, 201),
        {"status": boat_resp.status_code, "body": boat_resp.text[:180]},
    )

    time.sleep(0.25)

    detail_after_resp = REQUESTS_SESSION.get(f"{base_url}/api/stones/{stone_id}", headers=headers(u1), timeout=20)
    detail_after = assert_ok(detail_after_resp, f"/api/stones/{stone_id}")
    after = extract_detail(detail_after)
    boat_after = to_int(after.get("boat_count", 0), 0)
    ripple_after = to_int(after.get("ripple_count", 0), 0)
    add_check(
        "boat_count_monotonic",
        boat_after >= boat_before + 1,
        {"before": boat_before, "after": boat_after},
    )
    add_check(
        "ripple_count_monotonic",
        ripple_after >= ripple_before + 1,
        {"before": ripple_before, "after": ripple_after},
    )

    boats_list_resp = REQUESTS_SESSION.get(
        f"{base_url}/api/stones/{stone_id}/boats?page=1&page_size=20",
        headers=headers(u1),
        timeout=20,
    )
    boats_list_body = assert_ok(boats_list_resp, f"/api/stones/{stone_id}/boats")
    boats_data = boats_list_body.get("data", {})
    boats: List[Any] = []
    if isinstance(boats_data, dict):
        raw = boats_data.get("boats")
        if isinstance(raw, list):
            boats = raw
    add_check("boats_list_nonempty", len(boats) >= 1, {"boats_count": len(boats)})

    report["total_checks"] = len(report["checks"])
    report["passed_checks"] = sum(1 for c in report["checks"] if c["ok"])
    report["pass_rate"] = round(report["passed_checks"] / max(1, report["total_checks"]), 4)
    return report


def macro_f1(confusion: Dict[str, Dict[str, int]], classes: Iterable[str]) -> float:
    class_list = list(classes)
    if not class_list:
        return 0.0
    f1_scores: List[float] = []
    for cls in class_list:
        tp = confusion.get(cls, {}).get(cls, 0)
        actual = sum(confusion.get(cls, {}).values())
        predicted = sum(confusion.get(real, {}).get(cls, 0) for real in class_list)
        fp = max(0, predicted - tp)
        fn = max(0, actual - tp)
        precision = tp / (tp + fp) if (tp + fp) > 0 else 0.0
        recall = tp / (tp + fn) if (tp + fn) > 0 else 0.0
        if precision + recall <= 0:
            f1_scores.append(0.0)
        else:
            f1_scores.append((2 * precision * recall) / (precision + recall))
    return sum(f1_scores) / len(f1_scores)


def accuracy_suite(base_url: str, ctx: AuthCtx) -> Dict[str, Any]:
    # 覆盖 8 类 mood + 极性 + 用户反馈中的关键句
    samples = [
        ("今天收到了礼物，心里暖暖的，特别开心。", "happy", "positive"),
        ("老师今天夸了我，我觉得自己被认可了。", "happy", "positive"),
        ("终于把任务做完了，整个人都放松下来。", "calm", "positive"),
        ("下雨天在窗边喝茶，心情很平静。", "calm", "positive"),
        ("虽然有点紧张，但我相信自己可以。", "anxious", "negative"),
        ("明天要考试了，我一直担心发挥不好。", "anxious", "negative"),
        ("今天被误解了，心里很难过。", "sad", "negative"),
        ("朋友离开了这个城市，我有点失落。", "sad", "negative"),
        ("我被连续打断，真的很生气。", "angry", "negative"),
        ("项目被莫名否定，我很愤怒。", "angry", "negative"),
        ("突然收到意外惊喜，我有点震惊。", "surprised", "positive"),
        ("居然这么快就完成了，出乎意料。", "surprised", "positive"),
        ("信息太多了，我现在有点懵。", "confused", "negative"),
        ("他说的话前后矛盾，我很困惑。", "confused", "negative"),
        ("今天就按计划做事，没什么特别情绪。", "neutral", "neutral"),
        ("天气一般，日程正常，心情平平。", "neutral", "neutral"),
        ("今天收到礼物，礼物很好看。", "happy", "positive"),
        ("老师今天夸了我。", "happy", "positive"),
        ("虽然结果一般，但我会继续努力。", "calm", "positive"),
        ("我不开心，也不难过，就是发呆。", "neutral", "neutral"),
        ("这次失败让我很受挫。", "sad", "negative"),
        ("我有点焦虑，脑子停不下来。", "anxious", "negative"),
        ("居然又临时改需求，真让人火大。", "angry", "negative"),
        ("这件事太突然了，我完全没反应过来。", "surprised", "positive"),
        ("到底该先做哪一件，我整个人都乱了。", "confused", "negative"),
        ("和朋友散步聊天后，心里安定了许多。", "calm", "positive"),
        ("今天没有好消息也没有坏消息。", "neutral", "neutral"),
        ("我真的很开心，像阳光照进心里。", "happy", "positive"),
        ("最近压力很大，胸口发紧。", "anxious", "negative"),
        ("看完消息后我沉默了很久，很难受。", "sad", "negative"),
        ("被连续否决后我有点恼火。", "angry", "negative"),
        ("突然接到电话说通过了，我惊到了。", "surprised", "positive"),
    ]
    critical_cases = [
        ("今天收到礼物，礼物很好看。", "happy", "positive"),
        ("老师今天夸了我。", "happy", "positive"),
        ("我现在有点焦虑，心跳很快。", "anxious", "negative"),
        ("朋友离开后我很难过。", "sad", "negative"),
        ("什么都没发生，心情一般。", "neutral", "neutral"),
    ]

    h = headers(ctx)
    total = 0
    mood_hit = 0
    polarity_hit = 0
    confusion: Dict[str, Dict[str, int]] = {}
    details: List[Dict[str, Any]] = []

    for text, expect_mood, expect_pol in samples:
        total += 1
        resp = REQUESTS_SESSION.post(
            f"{base_url}/api/edge-ai/analyze",
            headers=h,
            json={"text": text},
            timeout=25,
        )
        if resp.status_code != 200:
            details.append(
                {
                    "text": text,
                    "expected_mood": expect_mood,
                    "expected_polarity": expect_pol,
                    "error": f"status={resp.status_code}",
                }
            )
            continue

        body = must_json(resp)
        data = body.get("data", {})
        pred_mood = str(data.get("mood", "neutral"))
        pred_score = float(data.get("score", 0.0))
        pred_pol = classify_polarity(pred_score, pred_mood)

        if pred_mood == expect_mood:
            mood_hit += 1
        if pred_pol == expect_pol:
            polarity_hit += 1

        confusion.setdefault(expect_mood, {})
        confusion[expect_mood][pred_mood] = confusion[expect_mood].get(pred_mood, 0) + 1
        details.append(
            {
                "text": text,
                "expected_mood": expect_mood,
                "pred_mood": pred_mood,
                "expected_polarity": expect_pol,
                "pred_polarity": pred_pol,
                "score": round(pred_score, 4),
                "ok_mood": pred_mood == expect_mood,
                "ok_polarity": pred_pol == expect_pol,
            }
        )

    critical_total = 0
    critical_hit = 0
    critical_detail: List[Dict[str, Any]] = []
    for text, expect_mood, expect_pol in critical_cases:
        critical_total += 1
        resp = REQUESTS_SESSION.post(
            f"{base_url}/api/edge-ai/analyze",
            headers=h,
            json={"text": text},
            timeout=25,
        )
        if resp.status_code != 200:
            critical_detail.append(
                {
                    "text": text,
                    "expected_mood": expect_mood,
                    "expected_polarity": expect_pol,
                    "error": f"status={resp.status_code}",
                    "ok": False,
                }
            )
            continue
        body = must_json(resp)
        data = body.get("data", {})
        pred_mood = str(data.get("mood", "neutral"))
        pred_score = float(data.get("score", 0.0))
        pred_pol = classify_polarity(pred_score, pred_mood)
        ok = (pred_mood == expect_mood) and (pred_pol == expect_pol)
        if ok:
            critical_hit += 1
        critical_detail.append(
            {
                "text": text,
                "expected_mood": expect_mood,
                "pred_mood": pred_mood,
                "expected_polarity": expect_pol,
                "pred_polarity": pred_pol,
                "score": round(pred_score, 4),
                "ok": ok,
            }
        )

    mood_acc = mood_hit / max(total, 1)
    pol_acc = polarity_hit / max(total, 1)
    return {
        "total": total,
        "mood_accuracy": round(mood_acc, 4),
        "polarity_accuracy": round(pol_acc, 4),
        "macro_f1": round(macro_f1(confusion, ALL_MOODS), 4),
        "critical_case_total": critical_total,
        "critical_case_hit": critical_hit,
        "critical_case_hit_rate": round(critical_hit / max(critical_total, 1), 4),
        "critical_case_detail": critical_detail,
        "confusion_matrix": confusion,
        "details": details,
    }


async def run_load(
    base_url: str,
    token: str,
    user_id: str,
    seconds: int,
    concurrency: int,
    mix_name: str,
) -> Dict[str, Any]:
    stop_at = time.perf_counter() + seconds
    latencies: List[float] = []
    status_counter: Dict[int, int] = {}
    errors = 0
    total = 0

    auth_headers = {"Authorization": f"Bearer {token}", "X-User-Id": user_id}

    if mix_name == "read_heavy":
        mix = [
            ("GET", "/api/lake/stones?page=1&page_size=20&sort=latest", None, 0.70),
            ("GET", "/api/recommendations/trending?limit=20", None, 0.15),
            ("GET", "/api/edge-ai/emotion-pulse", None, 0.10),
            ("GET", "/api/guardian/stats", None, 0.05),
        ]
    else:
        mix = [
            ("GET", "/api/lake/stones?page=1&page_size=20&sort=latest", None, 0.45),
            ("POST", "/api/edge-ai/analyze", {"text": "今天收到礼物，感觉很开心。"}, 0.30),
            ("GET", "/api/recommendations/trending?limit=20", None, 0.15),
            ("POST", "/api/edge-ai/emotion-sample", {"score": 0.77, "mood": "happy"}, 0.10),
        ]

    weighted: List[Tuple[str, str, Any]] = []
    for method, path, body, prob in mix:
        weighted.extend([(method, path, body)] * max(1, int(prob * 100)))
    if not weighted:
        raise RuntimeError("invalid load mix")

    async with httpx.AsyncClient(base_url=base_url, timeout=20.0, trust_env=HTTPX_TRUST_ENV) as client:
        async def worker() -> None:
            nonlocal errors, total
            while time.perf_counter() < stop_at:
                method, path, body = random.choice(weighted)
                t0 = time.perf_counter()
                try:
                    resp = await client.request(method, path, headers=auth_headers, json=body)
                    dt = (time.perf_counter() - t0) * 1000.0
                    latencies.append(dt)
                    status_counter[resp.status_code] = status_counter.get(resp.status_code, 0) + 1
                    total += 1
                except Exception:
                    errors += 1
                    total += 1

        workers = [asyncio.create_task(worker()) for _ in range(concurrency)]
        await asyncio.gather(*workers)

    ok_2xx = sum(v for k, v in status_counter.items() if 200 <= k < 300)
    err_5xx = sum(v for k, v in status_counter.items() if k >= 500)
    err_4xx = sum(v for k, v in status_counter.items() if 400 <= k < 500)

    def percentile(values: List[float], p: float) -> float:
        if not values:
            return 0.0
        values_sorted = sorted(values)
        idx = min(len(values_sorted) - 1, max(0, int(math.ceil(p * len(values_sorted))) - 1))
        return values_sorted[idx]

    duration = max(0.001, seconds)
    return {
        "mix": mix_name,
        "duration_sec": seconds,
        "concurrency": concurrency,
        "total_requests": total,
        "success_2xx": ok_2xx,
        "errors_transport": errors,
        "errors_4xx": err_4xx,
        "errors_5xx": err_5xx,
        "error_rate": round((errors + err_4xx + err_5xx) / max(total, 1), 4),
        "rps": round(total / duration, 2),
        "latency_ms": {
            "p50": round(percentile(latencies, 0.50), 2),
            "p95": round(percentile(latencies, 0.95), 2),
            "p99": round(percentile(latencies, 0.99), 2),
            "avg": round(statistics.mean(latencies), 2) if latencies else 0.0,
        },
        "status_breakdown": dict(sorted(status_counter.items())),
    }


def health_ok(base_url: str) -> bool:
    try:
        resp = REQUESTS_SESSION.get(f"{base_url}/api/health", timeout=5)
        return resp.status_code == 200
    except Exception:
        return False


async def run_stress_suite(
    base_url: str,
    token: str,
    user_id: str,
    seconds: int,
    levels: List[int],
) -> List[Dict[str, Any]]:
    results: List[Dict[str, Any]] = []
    for concurrency in levels:
        for mix in ("read_heavy", "ai_mixed"):
            item = await run_load(base_url, token, user_id, seconds, concurrency, mix)
            item["health_after"] = health_ok(base_url)
            results.append(item)
    return results


def quic_health(quic_host: str, quic_port: int) -> Dict[str, Any]:
    udp_ok = False
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(0.5)
    try:
        # 仅检测 UDP 端口可达（不做握手）
        sock.sendto(b"health_probe", (quic_host, quic_port))
        udp_ok = True
    except Exception:
        udp_ok = False
    finally:
        sock.close()
    return {"host": quic_host, "port": quic_port, "udp_send_ok": udp_ok}


def evaluate_gates(report: Dict[str, Any]) -> Dict[str, Any]:
    func = report["functional"]
    sec = report["security"]
    cons = report["consistency"]
    acc = report["accuracy"]
    stress = report["stress"]
    quic = report["quic"]

    stress_error_rate_max = max((float(item["error_rate"]) for item in stress), default=1.0)
    stress_p95_ms_max = max((float(item["latency_ms"]["p95"]) for item in stress), default=999999.0)
    stress_health_all_ok = all(bool(item.get("health_after", False)) for item in stress)

    checks = [
        {
            "name": "functional_pass_rate",
            "actual": func["pass_rate"],
            "target": GATE_TARGETS["functional_pass_rate"],
            "ok": func["pass_rate"] >= GATE_TARGETS["functional_pass_rate"],
        },
        {
            "name": "security_pass_rate",
            "actual": sec["pass_rate"],
            "target": GATE_TARGETS["security_pass_rate"],
            "ok": sec["pass_rate"] >= GATE_TARGETS["security_pass_rate"],
        },
        {
            "name": "consistency_pass_rate",
            "actual": cons["pass_rate"],
            "target": GATE_TARGETS["consistency_pass_rate"],
            "ok": cons["pass_rate"] >= GATE_TARGETS["consistency_pass_rate"],
        },
        {
            "name": "mood_accuracy",
            "actual": acc["mood_accuracy"],
            "target": GATE_TARGETS["mood_accuracy"],
            "ok": acc["mood_accuracy"] >= GATE_TARGETS["mood_accuracy"],
        },
        {
            "name": "macro_f1",
            "actual": acc["macro_f1"],
            "target": GATE_TARGETS["macro_f1"],
            "ok": acc["macro_f1"] >= GATE_TARGETS["macro_f1"],
        },
        {
            "name": "polarity_accuracy",
            "actual": acc["polarity_accuracy"],
            "target": GATE_TARGETS["polarity_accuracy"],
            "ok": acc["polarity_accuracy"] >= GATE_TARGETS["polarity_accuracy"],
        },
        {
            "name": "critical_case_hit_rate",
            "actual": acc["critical_case_hit_rate"],
            "target": GATE_TARGETS["critical_case_hit_rate"],
            "ok": acc["critical_case_hit_rate"] >= GATE_TARGETS["critical_case_hit_rate"],
        },
        {
            "name": "stress_error_rate_max",
            "actual": round(stress_error_rate_max, 4),
            "target": GATE_TARGETS["stress_error_rate_max"],
            "ok": stress_error_rate_max <= GATE_TARGETS["stress_error_rate_max"],
        },
        {
            "name": "stress_p95_ms_max",
            "actual": round(stress_p95_ms_max, 2),
            "target": GATE_TARGETS["stress_p95_ms_max"],
            "ok": stress_p95_ms_max <= GATE_TARGETS["stress_p95_ms_max"],
        },
        {
            "name": "stress_health_all_ok",
            "actual": stress_health_all_ok,
            "target": True,
            "ok": stress_health_all_ok,
        },
        {
            "name": "quic_udp_send_ok",
            "actual": bool(quic["udp_send_ok"]),
            "target": GATE_TARGETS["quic_udp_send_ok"],
            "ok": bool(quic["udp_send_ok"]) is GATE_TARGETS["quic_udp_send_ok"],
        },
    ]

    passed = sum(1 for c in checks if c["ok"])
    total = len(checks)
    score = round(passed / max(total, 1), 4)

    if score == 1.0:
        verdict = "门槛全部通过：当前样本集下达到评审级稳定标准"
    elif score >= 0.8:
        verdict = "接近评审级：仍存在关键短板，需要针对性补强"
    else:
        verdict = "未达到评审级门槛：核心指标存在明显不足"

    return {"checks": checks, "passed": passed, "total": total, "score": score, "verdict": verdict}


def render_markdown(report: Dict[str, Any]) -> str:
    func = report["functional"]
    sec = report["security"]
    cons = report["consistency"]
    acc = report["accuracy"]
    stress = report["stress"]
    quic = report["quic"]
    gates = report["gates"]

    lines = [
        "# HeartLake 评审级验证报告",
        "",
        f"- 生成时间: {report['generated_at']}",
        f"- 基础地址: `{report['base_url']}`",
        "",
        "## 1) 功能回归",
        f"- 功能通过率: **{func['passed_checks']}/{func['total_checks']} ({func['pass_rate'] * 100:.2f}%)**",
        f"- 失败数: **{len(func['failed'])}**",
        "",
        "## 2) 安全与契约",
        f"- 安全检查通过率: **{sec['passed_checks']}/{sec['total_checks']} ({sec['pass_rate'] * 100:.2f}%)**",
        f"- 数据一致性通过率: **{cons['passed_checks']}/{cons['total_checks']} ({cons['pass_rate'] * 100:.2f}%)**",
        "",
        "## 3) 情感精度",
        f"- Mood Top-1 准确率: **{acc['mood_accuracy'] * 100:.2f}%**",
        f"- Mood Macro-F1: **{acc['macro_f1'] * 100:.2f}%**",
        f"- 极性准确率: **{acc['polarity_accuracy'] * 100:.2f}%**",
        f"- 关键句命中率: **{acc['critical_case_hit']}/{acc['critical_case_total']} ({acc['critical_case_hit_rate'] * 100:.2f}%)**",
        "",
        "## 4) 压力与稳定性",
    ]

    for item in stress:
        lat = item["latency_ms"]
        lines.extend(
            [
                f"### {item['mix']} / 并发 {item['concurrency']}",
                f"- 时长: {item['duration_sec']}s",
                f"- RPS: **{item['rps']}**",
                f"- 错误率: **{item['error_rate'] * 100:.2f}%**",
                f"- 延迟: p50={lat['p50']}ms, p95={lat['p95']}ms, p99={lat['p99']}ms, avg={lat['avg']}ms",
                f"- 压测后健康检查: **{item['health_after']}**",
                "",
            ]
        )

    lines.extend(
        [
            "## 5) QUIC 通道",
            f"- UDP 发送探测: **{quic['udp_send_ok']}** (`{quic['host']}:{quic['port']}`)",
            "",
            "## 6) 严格门槛判定",
            f"- 通过门槛: **{gates['passed']}/{gates['total']} ({gates['score'] * 100:.2f}%)**",
            f"- 结论: **{gates['verdict']}**",
            "",
        ]
    )
    for gate in gates["checks"]:
        lines.append(
            f"- `{gate['name']}`: actual={gate['actual']} target={gate['target']} pass={gate['ok']}"
        )

    lines.extend(
        [
            "",
            "## 说明",
            "- 该报告提供“可复现证据链”，但无法在单机本地直接证明“优于世界上所有项目”。",
            "- 若要形成国际评审级对外论证，需补齐跨机器、跨地区、公开基线对比的第三方实验。",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-url", default="http://127.0.0.1:8080")
    parser.add_argument("--stress-seconds", type=int, default=20)
    parser.add_argument("--stress-levels", default="16,32,64")
    parser.add_argument("--quic-host", default="127.0.0.1")
    parser.add_argument("--quic-port", type=int, default=8443)
    parser.add_argument("--out-json", default="docs/世界级验证报告.json")
    parser.add_argument("--out-md", default="docs/世界级验证报告.md")
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()
    configure_http_clients(args.base_url)

    levels = parse_levels(args.stress_levels)

    health = REQUESTS_SESSION.get(f"{args.base_url}/api/health", timeout=10)
    if health.status_code != 200:
        raise RuntimeError(f"backend not healthy: status={health.status_code}, body={health.text[:200]}")

    u1 = auth_anonymous(args.base_url, "worldclass-u1")
    u2 = auth_anonymous(args.base_url, "worldclass-u2")

    functional = functional_suite(args.base_url, u1, u2)
    security = security_suite(args.base_url, u1)
    consistency = consistency_suite(args.base_url, u1, u2)
    accuracy = accuracy_suite(args.base_url, u1)
    stress = asyncio.run(
        run_stress_suite(args.base_url, u1.token, u1.user_id, args.stress_seconds, levels)
    )
    quic = quic_health(args.quic_host, args.quic_port)

    report = {
        "generated_at": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "base_url": args.base_url,
        "functional": functional,
        "security": security,
        "consistency": consistency,
        "accuracy": accuracy,
        "stress": stress,
        "quic": quic,
    }
    report["gates"] = evaluate_gates(report)

    with open(args.out_json, "w", encoding="utf-8") as f:
        json.dump(report, f, ensure_ascii=False, indent=2)
    with open(args.out_md, "w", encoding="utf-8") as f:
        f.write(render_markdown(report))

    stress_error_max = max((float(item["error_rate"]) for item in stress), default=1.0)
    stress_p95_max = max((float(item["latency_ms"]["p95"]) for item in stress), default=999999.0)

    summary = {
        "functional_pass_rate": functional["pass_rate"],
        "security_pass_rate": security["pass_rate"],
        "consistency_pass_rate": consistency["pass_rate"],
        "mood_accuracy": accuracy["mood_accuracy"],
        "macro_f1": accuracy["macro_f1"],
        "polarity_accuracy": accuracy["polarity_accuracy"],
        "critical_case_hit_rate": accuracy["critical_case_hit_rate"],
        "stress_error_rate_max": round(stress_error_max, 4),
        "stress_p95_ms_max": round(stress_p95_max, 2),
        "quic_udp_send_ok": quic["udp_send_ok"],
        "gate_passed": report["gates"]["passed"],
        "gate_total": report["gates"]["total"],
        "gate_score": report["gates"]["score"],
    }
    print(json.dumps(summary, ensure_ascii=False, indent=2))

    if args.strict and report["gates"]["passed"] != report["gates"]["total"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
