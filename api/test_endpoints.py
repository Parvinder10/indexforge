"""
IndexForge API Endpoints Automated Smoke Test
"""
import urllib.request
import urllib.parse
import json

BASE_URL = "http://127.0.0.1:8000"

def get(endpoint, params=None):
    url = BASE_URL + endpoint
    if params:
        url += "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode("utf-8"))

def main():
    print("Testing /api/health...")
    health = get("/api/health")
    assert health["status"] == "healthy", f"Health status: {health}"
    print(f"  Status: {health['status']}, Engine: {health['engine']}")

    print("Testing /api/stats...")
    stats = get("/api/stats")
    assert stats["total_documents"] > 0, f"No documents in index: {stats}"
    print(f"  Documents: {stats['total_documents']}, Unique Terms: {stats['total_unique_terms']}")

    test_queries = [
        "tender",
        '"public tender"',
        "tender AND modernization",
        "(security OR cloud) AND federal",
        "compliance NOT legacy"
    ]

    for q in test_queries:
        res = get("/api/search", {"q": q, "algo": "bm25"})
        print(f"  Query [{q}]: {res['total_hits']} hits, latency: {res['total_latency_ms']:.2f} ms")
        assert res["total_hits"] >= 0

    print("All API smoke tests passed successfully!")

if __name__ == "__main__":
    main()
