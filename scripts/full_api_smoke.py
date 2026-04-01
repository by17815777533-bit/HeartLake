#!/usr/bin/env python3
import argparse
import base64
import json
import os
import secrets
import sys
import tempfile
import time
from dataclasses import dataclass
from typing import Any, Callable, Dict, Optional, Sequence, Set

import requests
from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305


PASETO_HEADER = "v4.local."
PNG_1X1_BASE64 = (
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAusB9sJ6vTgAAAAASUVORK5CYII="
)


def b64url_no_padding(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).decode("ascii").rstrip("=")


def maybe_json(response: requests.Response) -> Optional[Any]:
    try:
        return response.json()
    except Exception:
        return None


def unwrap_data(payload: Any) -> Any:
    if isinstance(payload, dict) and "data" in payload:
        return payload["data"]
    return payload


def find_first(mapping: Dict[str, Any], keys: Sequence[str], default: Any = None) -> Any:
    for key in keys:
        if key in mapping and mapping[key] not in (None, ""):
            return mapping[key]
    return default


def build_admin_token(admin_key: str, admin_id: str = "admin_001", role: str = "super_admin") -> str:
    key = admin_key.encode("utf-8")[:32]
    if len(key) < 32:
        raise ValueError("ADMIN_PASETO_KEY must be at least 32 bytes")

    now = int(time.time())
    payload = json.dumps(
        {
            "sub": admin_id,
            "role": role,
            "iss": "heart_lake_admin",
            "iat": now,
            "exp": now + 24 * 3600,
        },
        ensure_ascii=False,
        separators=(",", ":"),
    ).encode("utf-8")

    nonce = os.urandom(12)
    token_body = nonce + ChaCha20Poly1305(key).encrypt(nonce, payload, b"")
    return PASETO_HEADER + b64url_no_padding(token_body)


@dataclass
class RouteResult:
    name: str
    method: str
    path: str
    status: int
    kind: str
    note: str = ""


class SmokeRunner:
    def __init__(self, base_url: str, timeout: float, admin_token: Optional[str]) -> None:
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self.admin_token = admin_token
        self.results: list[RouteResult] = []
        self.ctx: Dict[str, Any] = {}

    def user_session(self, token: Optional[str], device_name: str = "Codex Smoke") -> requests.Session:
        session = requests.Session()
        session.headers.update(
            {
                "User-Agent": "HeartLakeSmoke/1.0",
                "X-Device-Name": device_name,
                "Accept": "application/json",
            }
        )
        if token:
            session.headers["Authorization"] = f"Bearer {token}"
        return session

    def request(
        self,
        name: str,
        method: str,
        path: str,
        *,
        session: requests.Session,
        ok_statuses: Set[int] | Sequence[int] = (200,),
        expected_statuses: Set[int] | Sequence[int] = (),
        params: Optional[Dict[str, Any]] = None,
        json_body: Optional[Any] = None,
        files: Optional[Any] = None,
        data: Optional[Any] = None,
        validator: Optional[Callable[[requests.Response, Any], tuple[bool, str]]] = None,
    ) -> tuple[requests.Response, Any]:
        url = f"{self.base_url}{path}"
        response = session.request(
            method,
            url,
            params=params,
            json=json_body if files is None else None,
            files=files,
            data=data,
            timeout=self.timeout,
        )
        payload = maybe_json(response)
        ok_statuses = set(ok_statuses)
        expected_statuses = set(expected_statuses)
        kind = "fail"
        note = ""

        if response.status_code in ok_statuses:
            kind = "pass"
            if validator is not None:
                valid, detail = validator(response, payload)
                if not valid:
                    kind = "fail"
                    note = detail
                elif detail:
                    note = detail
        elif response.status_code in expected_statuses:
            kind = "expected_rejection"
            note = payload.get("message", "") if isinstance(payload, dict) else ""
        else:
            if isinstance(payload, dict):
                note = payload.get("message", "") or payload.get("error", "")
            if not note:
                note = response.text[:200]

        self.results.append(RouteResult(name, method, path, response.status_code, kind, note))
        return response, payload

    def skip(self, name: str, method: str, path: str, reason: str) -> None:
        self.results.append(RouteResult(name, method, path, 0, "skipped", reason))

    def must(
        self,
        name: str,
        method: str,
        path: str,
        *,
        session: requests.Session,
        ok_statuses: Set[int] | Sequence[int] = (200,),
        expected_statuses: Set[int] | Sequence[int] = (),
        params: Optional[Dict[str, Any]] = None,
        json_body: Optional[Any] = None,
        files: Optional[Any] = None,
        data: Optional[Any] = None,
        validator: Optional[Callable[[requests.Response, Any], tuple[bool, str]]] = None,
    ) -> tuple[requests.Response, Any]:
        response, payload = self.request(
            name,
            method,
            path,
            session=session,
            ok_statuses=ok_statuses,
            expected_statuses=expected_statuses,
            params=params,
            json_body=json_body,
            files=files,
            data=data,
            validator=validator,
        )
        if self.results[-1].kind != "pass":
            raise RuntimeError(f"{name} failed with status {response.status_code}: {self.results[-1].note}")
        return response, payload

    def create_user(self, label: str) -> Dict[str, Any]:
        device_id = f"codex-smoke-{label}-{int(time.time())}-{secrets.token_hex(3)}"
        session = self.user_session(None, f"Codex Smoke {label.upper()}")
        _, payload = self.must(
            f"auth_anonymous_{label}",
            "POST",
            "/api/auth/anonymous",
            session=session,
            json_body={"device_id": device_id},
            validator=lambda _r, p: (
                isinstance(unwrap_data(p), dict)
                and all(k in unwrap_data(p) for k in ("user_id", "token", "refresh_token", "session_id")),
                "anonymous login payload missing required fields",
            ),
        )
        data = unwrap_data(payload)
        token = data["token"]
        return {
            "label": label,
            "device_id": device_id,
            "session": self.user_session(token, f"Codex Smoke {label.upper()}"),
            "token": token,
            "user_id": data["user_id"],
            "refresh_token": data["refresh_token"],
            "session_id": data["session_id"],
            "recovery_key": data.get("recovery_key", ""),
        }

    def summary(self) -> Dict[str, int]:
        counts = {"pass": 0, "expected_rejection": 0, "fail": 0, "skipped": 0}
        for item in self.results:
            counts[item.kind] = counts.get(item.kind, 0) + 1
        return counts


def has_collection_items(payload: Any, *keys: str) -> list[Any]:
    data = unwrap_data(payload)
    if not isinstance(data, dict):
        return []
    for key in keys:
        value = data.get(key)
        if isinstance(value, list):
            return value
    return []


def extract_stone_ids(payload: Any) -> list[str]:
    ids: list[str] = []
    seen: set[str] = set()

    def visit(value: Any) -> None:
        if isinstance(value, dict):
            stone_id = find_first(value, ("stone_id", "id"))
            if isinstance(stone_id, str) and stone_id.startswith("stone_") and stone_id not in seen:
                seen.add(stone_id)
                ids.append(stone_id)
            for nested in value.values():
                visit(nested)
        elif isinstance(value, list):
            for item in value:
                visit(item)

    visit(unwrap_data(payload))
    return ids


def main() -> int:
    parser = argparse.ArgumentParser(description="HeartLake full API smoke runner")
    parser.add_argument("--base-url", default=os.environ.get("HEARTLAKE_BASE_URL", "http://121.41.195.165"))
    parser.add_argument("--timeout", type=float, default=20.0)
    parser.add_argument("--report-json", default="")
    args = parser.parse_args()

    admin_key = os.environ.get("ADMIN_PASETO_KEY", "").strip()
    admin_token = build_admin_token(admin_key) if admin_key else None
    smoke = SmokeRunner(args.base_url, args.timeout, admin_token)

    public = smoke.user_session(None)
    admin = smoke.user_session(admin_token, "Codex Smoke Admin") if admin_token else None

    try:
        smoke.request("health", "GET", "/api/health", session=public, ok_statuses={200})
        smoke.request("health_detailed", "GET", "/api/health/detailed", session=public, ok_statuses={200})

        user_a = smoke.create_user("a")
        user_b = smoke.create_user("b")
        user_c = smoke.create_user("c")
        user_d = smoke.create_user("d")
        user_e = smoke.create_user("e")
        user_f = smoke.create_user("f")
        smoke.ctx.update(
            {
                "user_a": user_a,
                "user_b": user_b,
                "user_c": user_c,
                "user_d": user_d,
                "user_e": user_e,
                "user_f": user_f,
            }
        )

        # Auth lifecycle
        _, recover_payload = smoke.must(
            "auth_recover_a",
            "POST",
            "/api/auth/recover",
            session=public,
            json_body={"recovery_key": user_a["recovery_key"], "device_id": user_a["device_id"] + "-recover"},
        )
        recovered = unwrap_data(recover_payload)
        smoke.ctx["user_a_recovered_session_id"] = recovered.get("session_id")
        smoke.ctx["user_a_recovered_token"] = recovered.get("token")
        smoke.must(
            "auth_refresh_a",
            "POST",
            "/api/auth/refresh",
            session=user_a["session"],
            json_body={"refresh_token": user_a["refresh_token"]},
            validator=lambda _r, p: (
                isinstance(unwrap_data(p), dict) and "token" in unwrap_data(p),
                "refresh payload missing token",
            ),
        )

        # Media + profile
        tmp_png = base64.b64decode(PNG_1X1_BASE64)
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as temp_file:
            temp_file.write(tmp_png)
            temp_path = temp_file.name
        try:
            with open(temp_path, "rb") as fh:
                _, media_payload = smoke.must(
                    "media_upload",
                    "POST",
                    "/api/media/upload",
                    session=user_a["session"],
                    files={"file": ("smoke.png", fh, "image/png")},
                )
            media_data = unwrap_data(media_payload)
            media_path = media_data.get("path")
            if media_path:
                smoke.request("media_serve", "GET", media_path, session=user_a["session"], ok_statuses={200})
        finally:
            try:
                os.unlink(temp_path)
            except OSError:
                pass

        smoke.request(
            "user_update_profile",
            "PUT",
            "/api/users/my/profile",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"nickname": "烟波测试A", "bio": "codex smoke profile"},
        )
        smoke.request(
            "account_update_avatar",
            "POST",
            "/api/account/avatar",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"avatar_url": media_path},
        )
        smoke.request(
            "account_update_profile",
            "PUT",
            "/api/account/profile",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"nickname": "烟波测试A2", "location": "Hangzhou", "bio": "account profile smoke"},
        )

        # Account and user info
        smoke.request("account_info", "GET", "/api/account/info", session=user_a["session"], ok_statuses={200})
        smoke.request("account_stats", "GET", "/api/account/stats", session=user_a["session"], ok_statuses={200})
        _, devices_payload = smoke.must(
            "account_devices",
            "GET",
            "/api/account/devices",
            session=user_a["session"],
            ok_statuses={200},
        )
        devices = has_collection_items(devices_payload, "devices")
        recover_session_id = smoke.ctx.get("user_a_recovered_session_id")
        if recover_session_id:
            smoke.request(
                "account_remove_device",
                "DELETE",
                f"/api/account/devices/{recover_session_id}",
                session=user_a["session"],
                ok_statuses={200},
            )
        else:
            smoke.skip("account_remove_device", "DELETE", "/api/account/devices/{session}", "missing secondary session")
        smoke.request("account_login_logs", "GET", "/api/account/login-logs", session=user_a["session"], ok_statuses={200})
        smoke.request("account_security_events", "GET", "/api/account/security-events", session=user_a["session"], ok_statuses={200})
        smoke.request("account_privacy_get", "GET", "/api/account/privacy", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "account_privacy_put",
            "PUT",
            "/api/account/privacy",
            session=user_a["session"],
            ok_statuses={200},
            json_body={
                "profile_visibility": "friends",
                "show_online_status": True,
                "allow_message_from_stranger": False,
            },
        )
        smoke.request(
            "account_privacy_put_c",
            "PUT",
            "/api/account/privacy",
            session=user_c["session"],
            ok_statuses={200},
            json_body={
                "profile_visibility": "public",
                "show_online_status": True,
                "allow_message_from_stranger": True,
            },
        )
        smoke.request("account_block_user", "POST", f"/api/account/block/{user_b['user_id']}", session=user_a["session"], ok_statuses={200})
        smoke.request("account_blocked_users", "GET", "/api/account/blocked-users?page=1&page_size=5", session=user_a["session"], ok_statuses={200})
        smoke.request("account_unblock_user", "DELETE", f"/api/account/unblock/{user_b['user_id']}", session=user_a["session"], ok_statuses={200})
        _, export_payload = smoke.must(
            "account_export",
            "POST",
            "/api/account/export",
            session=user_a["session"],
            ok_statuses={200},
        )
        export_data = unwrap_data(export_payload)
        export_id = find_first(export_data, ("task_id", "export_id", "id"))
        if export_id:
            smoke.request("account_export_status", "GET", f"/api/account/export/{export_id}", session=user_a["session"], ok_statuses={200})
        else:
            smoke.skip("account_export_status", "GET", "/api/account/export/{id}", "export task id missing")

        smoke.request("user_info_a", "GET", f"/api/users/{user_a['user_id']}", session=user_b["session"], ok_statuses={200})
        smoke.request("user_stats_a", "GET", f"/api/users/{user_a['user_id']}/stats", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "user_search",
            "GET",
            "/api/users/search",
            session=user_a["session"],
            ok_statuses={200},
            params={"q": user_b["user_id"][:8], "page": 1, "page_size": 5},
        )

        # Stones
        stone_common = {"stone_type": "medium", "stone_color": "#7A92A3", "is_anonymous": True, "tags": ["smoke", "api"]}
        _, stone_a_payload = smoke.must(
            "stone_create_main",
            "POST",
            "/api/stones",
            session=user_a["session"],
            ok_statuses={200},
            json_body={**stone_common, "content": f"[smoke] 主石头 {int(time.time())}", "mood_type": "hopeful"},
        )
        stone_a = unwrap_data(stone_a_payload)["stone_id"]
        _, stone_delete_payload = smoke.must(
            "stone_create_user_delete",
            "POST",
            "/api/stones",
            session=user_a["session"],
            ok_statuses={200},
            json_body={**stone_common, "content": f"[smoke] 用户删除石头 {int(time.time())}", "mood_type": "calm"},
        )
        stone_user_delete = unwrap_data(stone_delete_payload)["stone_id"]
        _, stone_admin_payload = smoke.must(
            "stone_create_admin_delete",
            "POST",
            "/api/stones",
            session=user_b["session"],
            ok_statuses={200},
            json_body={**stone_common, "content": f"[smoke] 管理员删除石头 {int(time.time())}", "mood_type": "grateful"},
        )
        stone_admin_delete = unwrap_data(stone_admin_payload)["stone_id"]

        _, stone_list_payload = smoke.must("stone_list", "GET", "/api/lake/stones?page=1&page_size=10", session=public, ok_statuses={200})
        smoke.request("stone_my", "GET", "/api/stones/my?page=1&page_size=10", session=user_a["session"], ok_statuses={200})
        smoke.request("stone_detail", "GET", f"/api/stones/{stone_a}", session=public, ok_statuses={200})
        smoke.request("lake_weather", "GET", "/api/lake/weather", session=public, ok_statuses={200})
        smoke.request("stone_delete_user", "DELETE", f"/api/stones/{stone_user_delete}", session=user_a["session"], ok_statuses={200})

        # Interactions
        _, ripple_payload = smoke.must(
            "interaction_create_ripple",
            "POST",
            f"/api/stones/{stone_a}/ripples",
            session=user_b["session"],
            ok_statuses={200},
        )
        ripple_id = find_first(unwrap_data(ripple_payload), ("ripple_id", "id"))
        _, boat_payload = smoke.must(
            "interaction_create_boat",
            "POST",
            f"/api/stones/{stone_a}/boats",
            session=user_b["session"],
            ok_statuses={200},
            json_body={"content": "[smoke] 来自 B 的纸船"},
        )
        boat_user_id = find_first(unwrap_data(boat_payload), ("boat_id", "id"))
        smoke.request("interaction_get_boats", "GET", f"/api/stones/{stone_a}/boats?page=1&page_size=10", session=user_a["session"], ok_statuses={200})
        _, notif_payload = smoke.must(
            "interaction_notifications",
            "GET",
            "/api/notifications?page=1&page_size=20",
            session=user_a["session"],
            ok_statuses={200},
        )
        notifications = has_collection_items(notif_payload, "notifications", "items")
        notification_id = ""
        if notifications:
            first_notification = notifications[0]
            if isinstance(first_notification, dict):
                notification_id = find_first(first_notification, ("notification_id", "id"), "")
        if notification_id:
            smoke.request("interaction_mark_notification_read", "POST", f"/api/notifications/{notification_id}/read", session=user_a["session"], ok_statuses={200})
        else:
            smoke.skip("interaction_mark_notification_read", "POST", "/api/notifications/{id}/read", "notification id missing")
        smoke.request("interaction_mark_all_notifications_read", "POST", "/api/notifications/read-all", session=user_a["session"], ok_statuses={200})
        smoke.request("interaction_unread_count", "GET", "/api/notifications/unread-count", session=user_a["session"], ok_statuses={200})
        _, connection_stone_payload = smoke.must(
            "interaction_create_connection_for_stone",
            "POST",
            f"/api/stones/{stone_a}/connections",
            session=user_b["session"],
            ok_statuses={200},
        )
        connection_stone_id = find_first(unwrap_data(connection_stone_payload), ("connection_id", "id"))
        _, connection_direct_payload = smoke.must(
            "interaction_create_connection",
            "POST",
            "/api/connections",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"target_user_id": user_c["user_id"]},
        )
        connection_direct_id = find_first(unwrap_data(connection_direct_payload), ("connection_id", "id"))
        if connection_direct_id:
            smoke.request(
                "interaction_connection_message_send",
                "POST",
                f"/api/connections/{connection_direct_id}/messages",
                session=user_a["session"],
                ok_statuses={200},
                json_body={"content": "[smoke] 连接消息"},
            )
            smoke.request(
                "interaction_connection_message_list",
                "GET",
                f"/api/connections/{connection_direct_id}/messages?page=1&page_size=20",
                session=user_a["session"],
                ok_statuses={200},
            )
            smoke.request(
                "interaction_connection_upgrade_friend",
                "POST",
                f"/api/connections/{connection_direct_id}/friend",
                session=user_a["session"],
                expected_statuses={400},
            )
        else:
            smoke.skip("interaction_connection_message_send", "POST", "/api/connections/{id}/messages", "connection id missing")
            smoke.skip("interaction_connection_message_list", "GET", "/api/connections/{id}/messages", "connection id missing")
            smoke.skip("interaction_connection_upgrade_friend", "POST", "/api/connections/{id}/friend", "connection id missing")
        smoke.request("interaction_my_ripples", "GET", "/api/interactions/my/ripples?page=1&page_size=10", session=user_b["session"], ok_statuses={200})
        smoke.request("interaction_my_boats", "GET", "/api/interactions/my/boats?page=1&page_size=10", session=user_b["session"], ok_statuses={200})

        # Paper boats
        _, paper_boat_payload = smoke.must(
            "paper_boat_reply",
            "POST",
            "/api/boats/reply",
            session=user_c["session"],
            ok_statuses={200},
            json_body={"stone_id": stone_a, "content": "[smoke] 来自 C 的纸船"},
        )
        boat_admin_id = find_first(unwrap_data(paper_boat_payload), ("boat_id", "id"))
        if boat_user_id:
            smoke.request("paper_boat_detail", "GET", f"/api/boats/{boat_user_id}", session=user_a["session"], ok_statuses={200})
        else:
            smoke.skip("paper_boat_detail", "GET", "/api/boats/{id}", "boat id missing")
        smoke.request("paper_boat_sent", "GET", "/api/boats/sent?page=1&page_size=10", session=user_b["session"], ok_statuses={200})
        smoke.request("paper_boat_received", "GET", "/api/boats/received?page=1&page_size=10", session=user_a["session"], ok_statuses={200})

        # Temp friend + friend
        smoke.request("temp_friend_create_downlined", "POST", "/api/temp-friends", session=user_a["session"], expected_statuses={400}, json_body={"target_user_id": user_b["user_id"]})
        _, temp_list_payload = smoke.request("temp_friend_list", "GET", "/api/temp-friends", session=user_a["session"], ok_statuses={200})
        temp_friends = has_collection_items(temp_list_payload, "temp_friends", "friends")
        temp_friend_id = ""
        if temp_friends and isinstance(temp_friends[0], dict):
            temp_friend_id = find_first(temp_friends[0], ("temp_friend_id", "id"), "")
        smoke.request("temp_friend_status", "GET", f"/api/temp-friends/check/{user_b['user_id']}", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "friend_remove",
            "DELETE",
            f"/api/friends/{user_b['user_id']}",
            session=user_a["session"],
            ok_statuses={200},
        )
        smoke.request(
            "friend_request_restore_hidden",
            "POST",
            "/api/friends/request",
            session=user_a["session"],
            ok_statuses={200, 400},
            expected_statuses={400},
            json_body={"target_user_id": user_b["user_id"]},
        )
        smoke.request("friend_accept_downlined", "POST", f"/api/friends/accept/{user_b['user_id']}", session=user_a["session"], expected_statuses={400})
        smoke.request("friend_reject_downlined", "POST", f"/api/friends/reject/{user_b['user_id']}", session=user_a["session"], expected_statuses={400})
        smoke.request("friend_list", "GET", "/api/friends", session=user_a["session"], ok_statuses={200})
        smoke.request("friend_pending_downlined", "GET", "/api/friends/requests/pending", session=user_a["session"], expected_statuses={400})
        smoke.request(
            "friend_send_message",
            "POST",
            f"/api/friends/{user_b['user_id']}/messages",
            session=user_a["session"],
            ok_statuses={200},
            expected_statuses={403},
            json_body={"content": "[smoke] 好友消息"},
        )
        smoke.request(
            "friend_get_messages",
            "GET",
            f"/api/friends/{user_b['user_id']}/messages",
            session=user_a["session"],
            ok_statuses={200},
            expected_statuses={403},
        )
        if temp_friend_id:
            smoke.request("temp_friend_detail", "GET", f"/api/temp-friends/{temp_friend_id}", session=user_a["session"], ok_statuses={200})
            smoke.request("temp_friend_upgrade_downlined", "POST", f"/api/temp-friends/{temp_friend_id}/upgrade", session=user_a["session"], expected_statuses={400})
            smoke.request("temp_friend_delete", "DELETE", f"/api/temp-friends/{temp_friend_id}", session=user_a["session"], ok_statuses={200, 404}, expected_statuses={404})
        else:
            smoke.request("temp_friend_detail_missing", "GET", "/api/temp-friends/temp_smoke_missing", session=user_a["session"], expected_statuses={404})
            smoke.request("temp_friend_upgrade_downlined_missing", "POST", "/api/temp-friends/temp_smoke_missing/upgrade", session=user_a["session"], expected_statuses={400})
            smoke.request("temp_friend_delete_missing", "DELETE", "/api/temp-friends/temp_smoke_missing", session=user_a["session"], expected_statuses={404})

        # Recommendation + vector
        _, recommend_stones_payload = smoke.must("recommend_stones", "GET", "/api/recommendations/stones?limit=5", session=user_a["session"], ok_statuses={200})
        smoke.request("recommend_discover_api", "GET", "/api/recommendations/discover/happy?limit=5", session=user_a["session"], ok_statuses={200})
        smoke.request("recommend_discover_compat", "GET", "/api/discover/happy?limit=5", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "recommend_track",
            "POST",
            "/api/recommendations/track",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"stone_id": stone_a, "action": "view", "duration_ms": 1500},
        )
        smoke.request("recommend_emotion_trends", "GET", "/api/recommendations/emotion-trends?days=7", session=user_a["session"], ok_statuses={200})
        _, recommend_trending_payload = smoke.must("recommend_trending", "GET", "/api/recommendations/trending?limit=5", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "recommend_search",
            "POST",
            "/api/recommendations/search",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"query": "温暖和共鸣", "limit": 5},
        )
        smoke.request(
            "recommend_track_batch",
            "POST",
            "/api/recommendations/track-batch",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"interactions": [{"stone_id": stone_a, "action": "ripple"}]},
        )
        _, recommend_advanced_payload = smoke.must("recommend_advanced", "GET", "/api/recommendations/advanced?limit=5", session=user_a["session"], ok_statuses={200})
        smoke.request("vector_personalized", "GET", "/api/recommendations/personalized?limit=5", session=user_a["session"], ok_statuses={200})

        if admin:
            smoke.request("admin_vector_update_embedding", "POST", f"/api/admin/stones/{stone_a}/embedding", session=admin, ok_statuses={200})
        else:
            smoke.skip("admin_vector_update_embedding", "POST", "/api/admin/stones/{id}/embedding", "missing ADMIN_PASETO_KEY")

        similar_candidates: list[str] = []
        for payload in (
            {"stone_id": stone_a},
            stone_list_payload,
            recommend_stones_payload,
            recommend_trending_payload,
            recommend_advanced_payload,
        ):
            for stone_id in extract_stone_ids(payload):
                if stone_id not in similar_candidates:
                    similar_candidates.append(stone_id)

        similar_response = None
        similar_payload = None
        similar_path = f"/api/recommendations/similar-stones/{stone_a}?limit=5"
        similar_note = "no embedded stone candidate available"
        for candidate in similar_candidates:
            detail_response = public.get(f"{smoke.base_url}/api/stones/{candidate}", timeout=smoke.timeout)
            if detail_response.status_code != 200:
                continue
            candidate_path = f"/api/recommendations/similar-stones/{candidate}?limit=5"
            response = user_a["session"].get(f"{smoke.base_url}{candidate_path}", timeout=smoke.timeout)
            payload = maybe_json(response)
            if response.status_code == 200:
                similar_response = response
                similar_payload = payload
                similar_path = candidate_path
                similar_note = ""
                break
            if response.status_code != 409:
                similar_response = response
                similar_payload = payload
                similar_path = candidate_path
                if isinstance(payload, dict):
                    similar_note = payload.get("message", "") or payload.get("error", "") or response.text[:200]
                else:
                    similar_note = response.text[:200]
                break
            similar_response = response
            similar_payload = payload
            similar_path = candidate_path
            if isinstance(payload, dict):
                similar_note = payload.get("message", "") or response.text[:200]
            else:
                similar_note = response.text[:200]

        if similar_response is not None and similar_response.status_code == 200:
            smoke.results.append(RouteResult("vector_similar_stones", "GET", similar_path, 200, "pass", similar_note))
        else:
            smoke.results.append(
                RouteResult(
                    "vector_similar_stones",
                    "GET",
                    similar_path,
                    0 if similar_response is None else similar_response.status_code,
                    "fail",
                    similar_note,
                )
            )
        smoke.request("stone_resonance", "GET", f"/api/stones/{stone_a}/resonance?limit=5", session=user_a["session"], ok_statuses={200})

        # Emotion visualization + privacy
        today = time.localtime()
        smoke.request(
            "user_emotion_calendar",
            "GET",
            f"/api/users/my/emotion-calendar?year={today.tm_year}&month={today.tm_mon}",
            session=user_a["session"],
            ok_statuses={200},
        )
        smoke.request("user_emotion_heatmap", "GET", "/api/users/my/emotion-heatmap?days=30", session=user_a["session"], ok_statuses={200})
        smoke.request("privacy_stats", "GET", "/api/lake/privacy-stats?epsilon=1.0", session=user_a["session"], ok_statuses={200})
        smoke.request("privacy_report", "GET", "/api/lake/privacy-report", session=user_a["session"], ok_statuses={200})

        # Safe harbor
        smoke.request("safe_harbor_hotlines", "GET", "/api/safe-harbor/hotlines", session=public, ok_statuses={200})
        smoke.request("safe_harbor_tools", "GET", "/api/safe-harbor/tools", session=public, ok_statuses={200})
        smoke.request("safe_harbor_prompt", "GET", "/api/safe-harbor/prompt?risk_level=low", session=public, ok_statuses={200})
        smoke.request("safe_harbor_resources", "GET", "/api/safe-harbor/resources", session=public, ok_statuses={200})
        smoke.request("safe_harbor_recommend", "GET", "/api/safe-harbor/recommend?mood=calm", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "safe_harbor_record_access",
            "POST",
            "/api/safe-harbor/access",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"resource_id": "page_view"},
        )
        smoke.request("safe_harbor_access_history", "GET", "/api/safe-harbor/access/history", session=user_a["session"], ok_statuses={200})

        # VIP + guardian
        smoke.request("vip_status", "GET", "/api/vip/status", session=user_a["session"], ok_statuses={200})
        smoke.request("vip_privileges", "GET", "/api/vip/privileges", session=user_a["session"], ok_statuses={200})
        smoke.request("vip_counseling_check", "GET", "/api/vip/counseling/check", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "vip_counseling_book",
            "POST",
            "/api/vip/counseling/book",
            session=user_a["session"],
            ok_statuses={200},
            expected_statuses={400},
            json_body={"appointment_time": "2026-04-01T10:00:00+08:00", "is_free_vip": False},
        )
        smoke.request("vip_ai_comment_frequency", "GET", "/api/vip/ai-comment-frequency", session=user_a["session"], ok_statuses={200})
        smoke.request("guardian_stats", "GET", "/api/guardian/stats", session=user_a["session"], ok_statuses={200})
        smoke.request("guardian_alias", "GET", "/api/guardian", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "guardian_transfer_lamp",
            "POST",
            "/api/guardian/transfer-lamp",
            session=user_a["session"],
            ok_statuses={200},
            expected_statuses={400},
            json_body={"to_user_id": user_b["user_id"]},
        )
        smoke.request("guardian_insights", "GET", "/api/guardian/insights", session=user_a["session"], ok_statuses={200})

        # Consultation
        _, consultation_payload = smoke.must(
            "consultation_create_session",
            "POST",
            "/api/consultation/session",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"counselor_id": user_c["user_id"]},
        )
        consultation_id = find_first(unwrap_data(consultation_payload), ("session_id", "id"))
        if consultation_id:
            smoke.request(
                "consultation_exchange_key",
                "POST",
                "/api/consultation/key-exchange",
                session=user_a["session"],
                ok_statuses={200},
                json_body={"session_id": consultation_id, "client_public_key": "smoke_public_key"},
            )
            smoke.request(
                "consultation_send_message",
                "POST",
                "/api/consultation/message",
                session=user_a["session"],
                ok_statuses={200},
                json_body={
                    "session_id": consultation_id,
                    "encrypted": {"ciphertext": "c21va2U=", "iv": "aXY=", "tag": "dGFn"},
                },
            )
            smoke.request(
                "consultation_get_messages",
                "GET",
                f"/api/consultation/messages/{consultation_id}?page=1&page_size=20",
                session=user_a["session"],
                ok_statuses={200},
            )
        else:
            smoke.skip("consultation_exchange_key", "POST", "/api/consultation/key-exchange", "session id missing")
            smoke.skip("consultation_send_message", "POST", "/api/consultation/message", "session id missing")
            smoke.skip("consultation_get_messages", "GET", "/api/consultation/messages/{id}", "session id missing")
        smoke.request("consultation_sessions", "GET", "/api/consultation/sessions?page=1&page_size=20", session=user_a["session"], ok_statuses={200})

        # Reports
        _, report_a_payload = smoke.must(
            "report_create_stone",
            "POST",
            "/api/reports",
            session=user_b["session"],
            ok_statuses={200},
            json_body={
                "target_type": "stone",
                "target_id": stone_a,
                "reason": "spam",
                "description": "[smoke] 举报石头",
            },
        )
        report_a = unwrap_data(report_a_payload)["report_id"]
        _, report_b_payload = smoke.must(
            "report_create_boat",
            "POST",
            "/api/reports",
            session=user_a["session"],
            ok_statuses={200},
            json_body={
                "target_type": "boat",
                "target_id": boat_user_id,
                "reason": "inappropriate",
                "description": "[smoke] 举报纸船",
            },
        )
        report_b = unwrap_data(report_b_payload)["report_id"]
        _, report_c_payload = smoke.must(
            "report_create_user",
            "POST",
            "/api/reports",
            session=user_c["session"],
            ok_statuses={200},
            json_body={
                "target_type": "user",
                "target_id": user_b["user_id"],
                "reason": "harassment",
                "description": "[smoke] 举报用户",
            },
        )
        report_c = unwrap_data(report_c_payload)["report_id"]
        smoke.request("report_my", "GET", "/api/reports/my?page=1&page_size=20", session=user_b["session"], ok_statuses={200})

        # Edge AI public
        smoke.request("edge_status", "GET", "/api/edge-ai/status", session=user_a["session"], ok_statuses={200})
        smoke.request("edge_metrics", "GET", "/api/edge-ai/metrics", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "edge_analyze",
            "POST",
            "/api/edge-ai/analyze",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"text": "今天有点难过，但我还是想继续走下去。"},
        )
        smoke.request(
            "edge_moderate",
            "POST",
            "/api/edge-ai/moderate",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"text": "这是一条普通的审核 smoke 文本。"},
        )
        smoke.request("edge_emotion_pulse", "GET", "/api/edge-ai/emotion-pulse", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "edge_federated_aggregate",
            "POST",
            "/api/edge-ai/federated/aggregate",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"round": 1, "minParticipants": 1, "epsilon": 1.0, "clippingBound": 1.0},
        )
        smoke.request("edge_privacy_budget", "GET", "/api/edge-ai/privacy-budget", session=user_a["session"], ok_statuses={200})
        smoke.request(
            "edge_vector_search",
            "POST",
            "/api/edge-ai/vector-search",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"query": "温柔 共鸣", "topK": 3},
        )
        smoke.request(
            "edge_vector_insert",
            "POST",
            "/api/edge-ai/vector-insert",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"id": f"smoke_vec_{secrets.token_hex(4)}", "vector": [0.1, 0.2, 0.3, 0.4]},
        )
        smoke.request(
            "edge_emotion_sample",
            "POST",
            "/api/edge-ai/emotion-sample",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"score": 0.2, "mood": "hopeful", "confidence": 0.8},
        )
        smoke.request(
            "edge_summary",
            "POST",
            "/api/edge-ai/summary",
            session=user_a["session"],
            ok_statuses={200},
            json_body={
                "stone_id": stone_a,
                "content": "这是一次完整 smoke 摘要测试，用来确认信息总结接口在长文本下能够正常返回，而且不会把响应结构打坏。"
                * 3,
            },
        )
        smoke.request(
            "guardian_chat",
            "POST",
            "/api/guardian/chat",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"content": "给我一句简短安慰。"},
        )
        smoke.request(
            "lake_god_chat",
            "POST",
            "/api/lake-god/chat",
            session=user_a["session"],
            ok_statuses={200},
            json_body={"content": "给我一句简短回复。"},
        )
        smoke.request("lake_god_history", "GET", "/api/lake-god/history", session=user_a["session"], ok_statuses={200})

        # Admin routes
        if admin:
            smoke.request(
                "admin_login_without_plaintext_secret",
                "POST",
                "/api/admin/login",
                session=public,
                expected_statuses={401},
                json_body={"username": "root", "password": "invalid-smoke-password"},
            )
            smoke.request("admin_logout", "POST", "/api/admin/logout", session=admin, ok_statuses={200})
            smoke.request("admin_info", "GET", "/api/admin/info", session=admin, ok_statuses={200})
            smoke.request("admin_trending_topics", "GET", "/api/admin/stats/trending-topics", session=admin, ok_statuses={200})
            smoke.request("admin_realtime_stats", "GET", "/api/admin/stats/realtime", session=admin, ok_statuses={200})
            smoke.request("admin_dashboard_stats", "GET", "/api/admin/stats/dashboard", session=admin, ok_statuses={200})
            smoke.request("admin_user_growth", "GET", "/api/admin/stats/user-growth?days=7", session=admin, ok_statuses={200})
            smoke.request("admin_mood_distribution", "GET", "/api/admin/stats/mood-distribution", session=admin, ok_statuses={200})
            smoke.request("admin_mood_trend", "GET", "/api/admin/stats/mood-trend?days=7", session=admin, ok_statuses={200})
            smoke.request("admin_active_time", "GET", "/api/admin/stats/active-time", session=admin, ok_statuses={200})
            smoke.request("admin_high_risk_users", "GET", "/api/admin/risk/high-risk-users?limit=10&offset=0", session=admin, ok_statuses={200})
            _, high_risk_events_payload = smoke.request("admin_high_risk_events", "GET", "/api/admin/risk/events?limit=10&offset=0", session=admin, ok_statuses={200})
            smoke.request("admin_user_risk_history", "GET", f"/api/admin/risk/user/{user_a['user_id']}/history", session=admin, ok_statuses={200})
            high_risk_events = has_collection_items(high_risk_events_payload, "events", "items")
            if high_risk_events and isinstance(high_risk_events[0], dict):
                risk_event_id = str(find_first(high_risk_events[0], ("event_id", "id"), ""))
                if risk_event_id:
                    smoke.request(
                        "admin_handle_risk_event",
                        "POST",
                        f"/api/admin/risk/event/{risk_event_id}/handle",
                        session=admin,
                        ok_statuses={200},
                        json_body={"action": "dismiss", "notes": "[smoke] dismiss"},
                    )
                else:
                    smoke.skip("admin_handle_risk_event", "POST", "/api/admin/risk/event/{id}/handle", "risk event id missing")
            else:
                smoke.skip("admin_handle_risk_event", "POST", "/api/admin/risk/event/{id}/handle", "no high risk event candidate")
            smoke.request("admin_security_audit", "GET", "/api/admin/security/audit", session=admin, ok_statuses={200})

            smoke.request("admin_users", "GET", "/api/admin/users?page=1&page_size=10", session=admin, ok_statuses={200})
            smoke.request("admin_user_detail", "GET", f"/api/admin/users/{user_a['user_id']}", session=admin, ok_statuses={200})
            smoke.request(
                "admin_update_user_status",
                "PUT",
                f"/api/admin/users/{user_c['user_id']}/status",
                session=admin,
                ok_statuses={200},
                json_body={"status": "active"},
            )
            smoke.request(
                "admin_ban_user",
                "POST",
                f"/api/admin/users/{user_c['user_id']}/ban",
                session=admin,
                ok_statuses={200},
                json_body={"reason": "[smoke] temporary ban"},
            )
            smoke.request("admin_unban_user", "POST", f"/api/admin/users/{user_c['user_id']}/unban", session=admin, ok_statuses={200})
            smoke.request("admin_content", "GET", "/api/admin/content?page=1&page_size=10", session=admin, ok_statuses={200})
            smoke.request("admin_stones", "GET", "/api/admin/stones?page=1&page_size=10", session=admin, ok_statuses={200})
            smoke.request("admin_stone_detail", "GET", f"/api/admin/stones/{stone_admin_delete}", session=admin, ok_statuses={200})
            smoke.request("admin_boats", "GET", "/api/admin/boats?page=1&page_size=10", session=admin, ok_statuses={200})
            smoke.request("admin_pending_moderation", "GET", "/api/admin/moderation/pending?page=1&page_size=10", session=admin, ok_statuses={200})
            smoke.request("admin_approve_content", "POST", f"/api/admin/moderation/{report_a}/approve", session=admin, ok_statuses={200})
            smoke.request(
                "admin_reject_content",
                "POST",
                f"/api/admin/moderation/{report_b}/reject",
                session=admin,
                ok_statuses={200},
                json_body={"reason": "[smoke] reject"},
            )
            smoke.request("admin_moderation_history", "GET", "/api/admin/moderation/history?page=1&page_size=10", session=admin, ok_statuses={200})
            smoke.request("admin_reports", "GET", "/api/admin/reports?page=1&page_size=10", session=admin, ok_statuses={200})
            smoke.request("admin_report_detail", "GET", f"/api/admin/reports/{report_c}", session=admin, ok_statuses={200})
            smoke.request(
                "admin_handle_report",
                "POST",
                f"/api/admin/reports/{report_c}/handle",
                session=admin,
                ok_statuses={200},
                json_body={"action": "ignored", "note": "[smoke] handle"},
            )
            _, sensitive_words_payload = smoke.request("admin_sensitive_words", "GET", "/api/admin/sensitive-words?page=1&page_size=10", session=admin, ok_statuses={200})
            _, add_sensitive_payload = smoke.must(
                "admin_sensitive_word_add",
                "POST",
                "/api/admin/sensitive-words",
                session=admin,
                ok_statuses={200},
                json_body={"word": f"smoke_word_{secrets.token_hex(4)}", "level": "medium", "category": "smoke"},
            )
            smoke.request("admin_sensitive_words_refresh", "GET", "/api/admin/sensitive-words?page=1&page_size=20", session=admin, ok_statuses={200})
            sensitive_words = has_collection_items(add_sensitive_payload, "words", "items")
            if sensitive_words and isinstance(sensitive_words[0], dict):
                sensitive_word_id = str(find_first(sensitive_words[0], ("id",), ""))
            else:
                words_after = has_collection_items(sensitive_words_payload, "words", "items")
                sensitive_word_id = ""
                for word in words_after:
                    if isinstance(word, dict) and str(word.get("word", "")).startswith("smoke_word_"):
                        sensitive_word_id = str(find_first(word, ("id",), ""))
                        break
            if sensitive_word_id:
                smoke.request(
                    "admin_sensitive_word_update",
                    "PUT",
                    f"/api/admin/sensitive-words/{sensitive_word_id}",
                    session=admin,
                    ok_statuses={200},
                    json_body={"word": f"smoke_word_updated_{secrets.token_hex(3)}", "level": "high", "category": "smoke"},
                )
                smoke.request(
                    "admin_sensitive_word_delete",
                    "DELETE",
                    f"/api/admin/sensitive-words/{sensitive_word_id}",
                    session=admin,
                    ok_statuses={200},
                )
            else:
                smoke.skip("admin_sensitive_word_update", "PUT", "/api/admin/sensitive-words/{id}", "sensitive word id missing")
                smoke.skip("admin_sensitive_word_delete", "DELETE", "/api/admin/sensitive-words/{id}", "sensitive word id missing")

            _, system_config_payload = smoke.must("admin_system_config", "GET", "/api/admin/config", session=admin, ok_statuses={200})
            system_config = unwrap_data(system_config_payload)
            smoke.request("admin_system_config_update", "PUT", "/api/admin/config", session=admin, ok_statuses={200}, json_body=system_config)
            smoke.skip("admin_broadcast", "POST", "/api/admin/broadcast", "skipped on live system to avoid pushing smoke message to online users")
            smoke.request("admin_broadcast_history", "GET", "/api/admin/broadcast/history?page=1&page_size=10", session=admin, ok_statuses={200})
            smoke.request("admin_logs", "GET", "/api/admin/logs?page=1&page_size=10", session=admin, ok_statuses={200})

            _, safe_resource_payload = smoke.must(
                "admin_safe_harbor_add_resource",
                "POST",
                "/api/safe-harbor/resources",
                session=admin,
                ok_statuses={200},
                json_body={"name": f"Smoke Resource {secrets.token_hex(3)}", "type": "tool"},
            )
            safe_resource_id = find_first(unwrap_data(safe_resource_payload), ("id",), "")
            if safe_resource_id:
                smoke.request(
                    "admin_safe_harbor_update_resource",
                    "PUT",
                    f"/api/safe-harbor/resources/{safe_resource_id}",
                    session=admin,
                    ok_statuses={200},
                    json_body={"name": "Smoke Resource Updated", "type": "tool"},
                )
                smoke.request(
                    "admin_safe_harbor_delete_resource",
                    "DELETE",
                    f"/api/safe-harbor/resources/{safe_resource_id}",
                    session=admin,
                    ok_statuses={204},
                )
            else:
                smoke.skip("admin_safe_harbor_update_resource", "PUT", "/api/safe-harbor/resources/{id}", "resource id missing")
                smoke.skip("admin_safe_harbor_delete_resource", "DELETE", "/api/safe-harbor/resources/{id}", "resource id missing")

            smoke.request("admin_recommend_advanced", "GET", f"/api/admin/recommendations/advanced?user_id={user_a['user_id']}&limit=5", session=admin, ok_statuses={200})

            smoke.request("admin_edge_status", "GET", "/api/admin/edge-ai/status", session=admin, ok_statuses={200})
            smoke.request("admin_edge_metrics", "GET", "/api/admin/edge-ai/metrics", session=admin, ok_statuses={200})
            smoke.request(
                "admin_edge_analyze",
                "POST",
                "/api/admin/edge-ai/analyze",
                session=admin,
                ok_statuses={200},
                json_body={"text": "管理员边缘AI分析 smoke"},
            )
            smoke.request(
                "admin_edge_moderate",
                "POST",
                "/api/admin/edge-ai/moderate",
                session=admin,
                ok_statuses={200},
                json_body={"text": "管理员边缘AI审核 smoke"},
            )
            smoke.request("admin_edge_emotion_pulse", "GET", "/api/admin/edge-ai/emotion-pulse", session=admin, ok_statuses={200})
            smoke.request(
                "admin_edge_federated_aggregate",
                "POST",
                "/api/admin/edge-ai/federated/aggregate",
                session=admin,
                ok_statuses={200},
                json_body={"round": 2, "minParticipants": 1, "epsilon": 1.0, "clippingBound": 1.0},
            )
            smoke.request("admin_edge_privacy_budget", "GET", "/api/admin/edge-ai/privacy-budget", session=admin, ok_statuses={200})
            smoke.request(
                "admin_edge_vector_search",
                "POST",
                "/api/admin/edge-ai/vector-search",
                session=admin,
                ok_statuses={200},
                json_body={"query": "共鸣 温暖", "topK": 3},
            )
            smoke.request(
                "admin_edge_vector_insert",
                "POST",
                "/api/admin/edge-ai/vector-insert",
                session=admin,
                ok_statuses={200},
                json_body={"id": f"admin_smoke_vec_{secrets.token_hex(4)}", "vector": [0.4, 0.3, 0.2, 0.1]},
            )
            smoke.request(
                "admin_edge_emotion_sample",
                "POST",
                "/api/admin/edge-ai/emotion-sample",
                session=admin,
                ok_statuses={200},
                json_body={"score": 0.3, "mood": "grateful", "confidence": 0.9},
            )
            smoke.request("admin_edge_pulse_history", "GET", "/api/admin/edge-ai/pulse-history?count=10", session=admin, ok_statuses={200})
            smoke.request(
                "admin_edge_submit_local_model",
                "POST",
                "/api/admin/edge-ai/federated/submit",
                session=admin,
                ok_statuses={200},
                json_body={
                    "model_id": "smoke-model",
                    "node_id": "smoke-node",
                    "sample_count": 5,
                    "local_loss": 0.2,
                    "epoch": 1,
                    "weights": [[0.1, 0.2], [0.3, 0.4]],
                    "biases": [0.1, 0.2],
                },
            )
            smoke.request("admin_edge_reset_privacy_budget", "POST", "/api/admin/edge-ai/privacy/reset", session=admin, ok_statuses={200})
            smoke.request(
                "admin_edge_register_node",
                "POST",
                "/api/admin/edge-ai/nodes/register",
                session=admin,
                ok_statuses={200},
                json_body={"node_id": "smoke-node"},
            )
            smoke.request(
                "admin_edge_update_node_status",
                "PUT",
                "/api/admin/edge-ai/nodes/status",
                session=admin,
                ok_statuses={200},
                json_body={
                    "node_id": "smoke-node",
                    "cpu_usage": 0.1,
                    "memory_usage": 0.2,
                    "latency_ms": 12.0,
                    "active_connections": 1,
                    "total_requests": 10,
                    "failed_requests": 0,
                    "is_healthy": True,
                },
            )
            smoke.request("admin_edge_best_node", "GET", "/api/admin/edge-ai/nodes/best", session=admin, ok_statuses={200})
            smoke.request(
                "admin_edge_quantized_forward",
                "POST",
                "/api/admin/edge-ai/quantized-forward",
                session=admin,
                ok_statuses={200},
                json_body={"input": [0.1, 0.2], "weights": [[0.1, 0.2], [0.3, 0.4]], "biases": [0.1, 0.2]},
            )
            smoke.request(
                "admin_edge_add_noise",
                "POST",
                "/api/admin/edge-ai/privacy/add-noise",
                session=admin,
                ok_statuses={200},
                json_body={"values": [1.0, 2.0, 3.0], "sensitivity": 1.0},
            )
            _, edge_config_payload = smoke.must("admin_edge_config_get", "GET", "/api/admin/edge-ai/config", session=admin, ok_statuses={200})
            edge_config = unwrap_data(edge_config_payload)
            smoke.request(
                "admin_edge_config_update",
                "PUT",
                "/api/admin/edge-ai/config",
                session=admin,
                ok_statuses={200},
                json_body={
                    "edge_ai_enabled": edge_config.get("edge_ai_enabled", True),
                    "dp_epsilon": edge_config.get("dp_epsilon", 1.0),
                    "hnsw_ef_search": edge_config.get("hnsw_ef_search", 50),
                },
            )

            smoke.request("admin_delete_stone", "DELETE", f"/api/admin/stones/{stone_admin_delete}", session=admin, ok_statuses={200})
            if boat_admin_id:
                smoke.request("admin_delete_boat", "DELETE", f"/api/admin/boats/{boat_admin_id}", session=admin, ok_statuses={200})
            else:
                smoke.skip("admin_delete_boat", "DELETE", "/api/admin/boats/{id}", "admin boat id missing")
        else:
            smoke.skip("admin_routes", "GET", "/api/admin/*", "missing ADMIN_PASETO_KEY")

        # Cleanup of user-owned interaction artifacts
        if boat_user_id:
            smoke.request("interaction_delete_boat", "DELETE", f"/api/boats/{boat_user_id}", session=user_b["session"], ok_statuses={200})
        else:
            smoke.skip("interaction_delete_boat", "DELETE", "/api/boats/{id}", "user boat id missing")
        if ripple_id:
            smoke.request("interaction_delete_ripple", "DELETE", f"/api/ripples/{ripple_id}", session=user_b["session"], ok_statuses={200})
        else:
            smoke.skip("interaction_delete_ripple", "DELETE", "/api/ripples/{id}", "ripple id missing")

        # Destructive auth/account lifecycle on disposable users
        smoke.request(
            "auth_delete_account_compat",
            "POST",
            "/api/auth/delete-account",
            session=user_d["session"],
            ok_statuses={200},
            json_body={"confirmation": "DEACTIVATE"},
        )
        smoke.request(
            "account_deactivate",
            "POST",
            "/api/account/deactivate",
            session=user_e["session"],
            ok_statuses={200},
            json_body={"confirmation": "DEACTIVATE"},
        )
        smoke.request(
            "account_delete_permanent",
            "POST",
            "/api/account/delete-permanent",
            session=user_f["session"],
            ok_statuses={200},
            json_body={"confirmation": "DELETE"},
        )
    except Exception as exc:
        print(f"FATAL: {exc}", file=sys.stderr)

    counts = smoke.summary()
    failures = [r for r in smoke.results if r.kind == "fail"]
    skipped = [r for r in smoke.results if r.kind == "skipped"]

    print(json.dumps(
        {
            "base_url": args.base_url,
            "counts": counts,
            "failures": [
                {
                    "name": r.name,
                    "method": r.method,
                    "path": r.path,
                    "status": r.status,
                    "note": r.note,
                }
                for r in failures
            ],
            "skipped": [
                {
                    "name": r.name,
                    "method": r.method,
                    "path": r.path,
                    "reason": r.note,
                }
                for r in skipped
            ],
        },
        ensure_ascii=False,
        indent=2,
    ))

    if args.report_json:
        with open(args.report_json, "w", encoding="utf-8") as fh:
            json.dump(
                {
                    "base_url": args.base_url,
                    "results": [r.__dict__ for r in smoke.results],
                    "counts": counts,
                },
                fh,
                ensure_ascii=False,
                indent=2,
            )

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
