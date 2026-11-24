#!/usr/bin/env python3
"""
IndexForge gRPC Smoke Test Client
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))

import grpc
import search_service_pb2 as pb2
import search_service_pb2_grpc as pb2_grpc

def main():
    channel = grpc.insecure_channel('localhost:50051')
    stub = pb2_grpc.SearchEngineServiceStub(channel)

    print("[*] Calling CheckHealth...")
    health = stub.CheckHealth(pb2.HealthRequest())
    print(f"  Health: status={health.status}, info={health.engine_info}")

    print("[*] Calling GetStats...")
    stats = stub.GetStats(pb2.StatsRequest())
    print(f"  Stats: active_docs={stats.active_documents}, unique_terms={stats.total_unique_terms}")

    print("[*] Calling Search for 'tender'...")
    search_res = stub.Search(pb2.SearchRequest(query="tender", algorithm=pb2.RankingAlgorithm.BM25, limit=5))
    print(f"  Search: total_hits={search_res.total_hits}, latency={search_res.engine_latency_ms} ms")
    for hit in search_res.hits:
        print(f"    - Doc #{hit.doc_id}: {hit.title} (Score: {hit.score:.3f})")

    print("[✓] All gRPC smoke tests passed successfully!")

if __name__ == "__main__":
    main()
