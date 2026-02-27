#!/usr/bin/env python3
"""
HeartLake API Load Tester
Performance audit: target API response time < 50ms
"""
import argparse
import json
import statistics
import subprocess
import sys
import time
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed

BASE_URL = "http://127.0.0.1:8080"
TARGET_MS = 50
CONCURRENCY = 50
REQUESTS_PER_ENDPOINT = 200
REQUEST_TIMEOUT_SECONDS = 10.0

ENDPOINTS = [
    ("GET", "/api/health", None),
    ("GET", "/api/lake/stones", None),
    ("GET", "/api/lake/weather", None),
    ("POST", "/api/auth/anonymous", '{"device_id":"load-test-device"}'),
]

def percentile(values, ratio):
    if not values:
        return 0.0
    idx = min(len(values) - 1, max(0, int((len(values) - 1) * ratio)))
    return values[idx]

def make_request(base_url, timeout_seconds, method, path, body):
    url = f"{base_url}{path}"
    start = time.perf_counter()
    cmd = ["curl", "-s", "-o", "/dev/null", "-w", "%{http_code}", "-X", method, url]
    if body is not None:
        cmd.extend(["-H", "Content-Type: application/json", "--data", body])
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout_seconds,
            check=False,
        )
        elapsed_ms = (time.perf_counter() - start) * 1000
        status = int(result.stdout.strip()) if result.stdout.strip().isdigit() else 0
        return elapsed_ms, status
    except Exception:
        return None, 0

def test_endpoint(base_url, concurrency, num_requests, timeout_seconds, method, path, body):
    times = []
    errors = 0
    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        futures = [
            executor.submit(make_request, base_url, timeout_seconds, method, path, body)
            for _ in range(num_requests)
        ]
        for f in as_completed(futures):
            elapsed, status = f.result()
            if elapsed and 200 <= status < 400:
                times.append(elapsed)
            else:
                errors += 1
    return times, errors

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-url", default=BASE_URL)
    parser.add_argument("--target-ms", type=float, default=TARGET_MS)
    parser.add_argument("--concurrency", type=int, default=CONCURRENCY)
    parser.add_argument("--requests-per-endpoint", type=int, default=REQUESTS_PER_ENDPOINT)
    parser.add_argument("--timeout-seconds", type=float, default=REQUEST_TIMEOUT_SECONDS)
    parser.add_argument(
        "--out",
        default=str(Path(__file__).with_name("load_test_results.json")),
    )
    return parser.parse_args()

def main():
    args = parse_args()

    print(f"HeartLake Load Test - Target: <{args.target_ms}ms avg response")
    print(
        f"Concurrency: {args.concurrency}, "
        f"Requests/endpoint: {args.requests_per_endpoint}"
    )
    print("-" * 60)

    all_passed = True
    results = []

    for method, path, body in ENDPOINTS:
        times, errors = test_endpoint(
            args.base_url,
            args.concurrency,
            args.requests_per_endpoint,
            args.timeout_seconds,
            method,
            path,
            body,
        )
        endpoint_name = f"{method} {path}"
        if not times:
            print(f"[FAIL] {endpoint_name} - no successful responses")
            all_passed = False
            results.append({
                "endpoint": endpoint_name,
                "errors": errors,
                "successful_requests": 0,
                "total_requests": args.requests_per_endpoint,
                "passed": False,
                "reason": "no_successful_responses",
            })
            continue

        avg = statistics.mean(times)
        p50 = statistics.median(times)
        sorted_times = sorted(times)
        p95 = percentile(sorted_times, 0.95)
        p99 = percentile(sorted_times, 0.99)

        passed = avg < args.target_ms and errors == 0
        status = "PASS" if passed else "FAIL"
        if not passed:
            all_passed = False

        print(f"[{status}] {endpoint_name}")
        print(
            f"       avg={avg:.1f}ms p50={p50:.1f}ms p95={p95:.1f}ms "
            f"p99={p99:.1f}ms errors={errors}"
        )

        results.append({
            "endpoint": endpoint_name,
            "avg_ms": round(avg, 2),
            "p50_ms": round(p50, 2),
            "p95_ms": round(p95, 2),
            "p99_ms": round(p99, 2),
            "errors": errors,
            "successful_requests": len(times),
            "total_requests": args.requests_per_endpoint,
            "passed": passed
        })

    print("-" * 60)
    print(f"Overall: {'PASS' if all_passed else 'FAIL'}")

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as f:
        json.dump(
            {
                "base_url": args.base_url,
                "target_ms": args.target_ms,
                "concurrency": args.concurrency,
                "requests_per_endpoint": args.requests_per_endpoint,
                "results": results,
                "passed": all_passed,
            },
            f,
            indent=2,
        )
    print(f"Saved results: {out_path}")

    return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(main())
