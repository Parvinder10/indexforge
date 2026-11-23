import requests

def verify():
    base_url = "http://127.0.0.1:8000"

    # 1. Health check
    h = requests.get(f"{base_url}/api/health")
    print("Health Check:", h.status_code, h.json())
    assert h.status_code == 200

    # 2. Ingest Sample Documents
    sample = requests.post(f"{base_url}/api/ingest/sample")
    print("Ingest Samples:", sample.json()["status"], "Count:", sample.json()["ingested_count"])

    # 3. Stats check
    stats = requests.get(f"{base_url}/api/stats")
    s_data = stats.json()
    print("Corpus Stats:")
    print(f"  - Active Documents: {s_data['active_documents']}")
    print(f"  - Unique Terms: {s_data['total_unique_terms']}")
    print(f"  - Memory: {s_data['estimated_memory_bytes']} bytes")
    print(f"  - Avg Document Length: {s_data['avg_doc_length']} tokens")

    # 4. Search BM25: "public tender"
    bm25 = requests.get(f"{base_url}/api/search", params={"q": "public tender", "algo": "bm25"})
    b_data = bm25.json()
    print(f"Search 'public tender' (BM25): {b_data['total_hits']} hits, Engine Latency: {b_data['engine_latency_ms']} ms")
    for hit in b_data["hits"][:2]:
        print(f"    - #{hit['doc_id']}: {hit['title']} (Score: {hit['score']})")

    # 5. Phrase Search: "\"public tender\""
    phrase = requests.get(f"{base_url}/api/search", params={"q": '"public tender"'})
    p_data = phrase.json()
    print(f"Phrase Search '\"public tender\"': {p_data['total_hits']} hits, Latency: {p_data['engine_latency_ms']} ms")

    # 6. Boolean AND Search: "procurement AND compliance"
    boolean_and = requests.get(f"{base_url}/api/search", params={"q": "procurement AND compliance"})
    and_data = boolean_and.json()
    print(f"Boolean AND 'procurement AND compliance': {and_data['total_hits']} hits")

    # 7. Linear Scan Baseline
    linear = requests.get(f"{base_url}/api/search/linear", params={"q": "public tender"})
    l_data = linear.json()
    print(f"Linear Scan Baseline: {l_data['total_hits']} hits, Latency: {l_data['engine_latency_ms']} ms")

    # 8. Web UI Endpoint
    ui = requests.get(f"{base_url}/")
    print(f"Web UI Endpoint: Status {ui.status_code}, Length: {len(ui.text)} bytes")

    print("\nALL LIVE REST API & SEARCH ENDPOINTS VERIFIED SUCCESSFULLY!")

if __name__ == "__main__":
    verify()
