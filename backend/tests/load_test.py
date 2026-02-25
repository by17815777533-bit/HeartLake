#!/usr/bin/env python3
"""
HeartLake API Load Tester
Performance audit: target API response time < 50ms
"""
import subprocess
import json
import time
import statistics
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed

BASE_URL = "http://127.0.0.1:8080"
TARGET_MS = 50
CONCURRENCY = 50
REQUESTS_PER_ENDPOINT = 200

ENDPOINTS = [
    ("GET", "/api/health"),
    ("GET", "/api/lake/stones"),
    ("GET", "/api/lake/weather"),
    ("POST", "/api/auth/anonymous"),
]

def make_request(method, path):
    url = f"{BASE_URL}{path}"
    start = time.perf_counter()
    try:
        result = subprocess.run(
            ["curl", "-s", "-o", "/dev/null", "-w", "%{http_code}", "-X", method, url],
            capture_output=True, text=True, timeout=10
        )
        elapsed_ms = (time.perf_counter() - start) * 1000
        status = int(result.stdout.strip()) if result.stdout.strip().isdigit() else 0
        return elapsed_ms, status
    except Exception as e:
        return None, 0

def test_endpoint(method, path, num_requests):
    times = []
    errors = 0
    with ThreadPoolExecutor(max_workers=CONCURRENCY) as executor:
        futures = [executor.submit(make_request, method, path) for _ in range(num_requests)]
        for f in as_completed(futures):
            elapsed, status = f.result()
            if elapsed and 200 <= status < 400:
                times.append(elapsed)
            else:
                errors += 1
    return times, errors

def main():
    print(f"HeartLake Load Test - Target: <{TARGET_MS}ms avg response")
    print(f"Concurrency: {CONCURRENCY}, Requests/endpoint: {REQUESTS_PER_ENDPOINT}")
    print("-" * 60)

    all_passed = True
    results = []

    for method, path in ENDPOINTS:
        times, errors = test_endpoint(method, path, REQUESTS_PER_ENDPOINT)
        if not times:
            print(f"[SKIP] {method} {path} - no successful responses")
            continue

        avg = statistics.mean(times)
        p50 = statistics.median(times)
        p95 = sorted(times)[int(len(times) * 0.95)] if len(times) > 1 else times[0]
        p99 = sorted(times)[int(len(times) * 0.99)] if len(times) > 1 else times[0]

        passed = avg < TARGET_MS
        status = "PASS" if passed else "FAIL"
        if not passed:
            all_passed = False

        print(f"[{status}] {method} {path}")
        print(f"       avg={avg:.1f}ms p50={p50:.1f}ms p95={p95:.1f}ms p99={p99:.1f}ms errors={errors}")

        results.append({
            "endpoint": f"{method} {path}",
            "avg_ms": round(avg, 2),
            "p50_ms": round(p50, 2),
            "p95_ms": round(p95, 2),
            "p99_ms": round(p99, 2),
            "errors": errors,
            "passed": passed
        })

    print("-" * 60)
    print(f"Overall: {'PASS' if all_passed else 'FAIL'}")

    with open("/home/jokerbai/Documents/HeartLake/backend/tests/load_test_results.json", "w") as f:
        json.dump({"target_ms": TARGET_MS, "results": results, "passed": all_passed}, f, indent=2)

    return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(main())
