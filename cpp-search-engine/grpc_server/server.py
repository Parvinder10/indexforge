#!/usr/bin/env python3
"""
IndexForge C++20 Search Engine — High-Performance gRPC Service Bridge
Listens on port 50051 and exposes the C++20 Inverted Index Core to Java Spring Boot.
"""

import os
import sys
import time
import argparse
import logging
from concurrent import futures
from pathlib import Path

# Add repo root and grpc_server to sys.path
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO_ROOT))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import grpc
from api.indexforge_binding import IndexForgeBinding
import search_service_pb2 as pb2
import search_service_pb2_grpc as pb2_grpc

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger("IndexForge-gRPC")

class SearchEngineServiceImpl(pb2_grpc.SearchEngineServiceServicer):
    def __init__(self, engine: IndexForgeBinding):
        self.engine = engine
        self.start_time = time.time()
        logger.info("[✓] C++20 Search Engine instance bound to gRPC service.")

    def IndexDocument(self, request, context):
        try:
            ok = self.engine.index_document(
                doc_id=request.doc_id,
                title=request.title,
                source=request.source,
                date=request.date,
                doc_type=request.doc_type,
                content=request.content
            )
            tokens = len(request.content.split())
            return pb2.IndexDocumentResponse(
                success=ok,
                doc_id=request.doc_id,
                tokens_indexed=tokens if ok else 0,
                error_message="" if ok else "Failed to index document in C++ core"
            )
        except Exception as e:
            logger.error(f"Error in IndexDocument: {e}")
            return pb2.IndexDocumentResponse(
                success=False,
                doc_id=request.doc_id,
                tokens_indexed=0,
                error_message=str(e)
            )

    def IndexBatch(self, request, context):
        indexed = 0
        results = []
        for doc_req in request.documents:
            res = self.IndexDocument(doc_req, context)
            results.append(res)
            if res.success:
                indexed += 1
        return pb2.IndexBatchResponse(
            success=True,
            indexed_count=indexed,
            results=results,
            error_message=""
        )

    def DeleteDocument(self, request, context):
        try:
            ok = self.engine.delete_document(request.doc_id)
            return pb2.DeleteDocumentResponse(
                success=ok,
                doc_id=request.doc_id,
                error_message="" if ok else f"Doc #{request.doc_id} not found"
            )
        except Exception as e:
            logger.error(f"Error in DeleteDocument: {e}")
            return pb2.DeleteDocumentResponse(
                success=False,
                doc_id=request.doc_id,
                error_message=str(e)
            )

    def Search(self, request, context):
        t0 = time.perf_counter()
        query = request.query.strip()
        if not query:
            return pb2.SearchResponse(
                query="",
                algorithm="none",
                total_hits=0,
                engine_latency_ms=0.0,
                from_cache=False,
                hits=[]
            )

        algo_str = "bm25"
        if request.algorithm == pb2.RankingAlgorithm.TF_IDF:
            algo_str = "tfidf"
        elif request.algorithm == pb2.RankingAlgorithm.LINEAR_SCAN:
            algo_str = "linear"

        limit = request.limit if request.limit > 0 else 20
        offset = request.offset
        k1 = request.k1 if request.k1 > 0 else 1.2
        b = request.b if request.b > 0 else 0.75

        try:
            if algo_str == "linear":
                raw_res = self.engine.linear_search(query, limit=limit + offset)
                hits = raw_res.get("hits", [])
                total_hits = raw_res.get("total_hits", len(hits))
                from_cache = False
                eng_latency = raw_res.get("latency_ms", 0.0)
            else:
                raw_res = self.engine.search(
                    query=query,
                    algo=algo_str,
                    k1=k1,
                    b=b,
                    limit=limit + offset,
                    offset=0
                )
                hits = raw_res.get("hits", [])
                total_hits = raw_res.get("total_hits", len(hits))
                from_cache = raw_res.get("from_cache", False)
                eng_latency = raw_res.get("latency_ms", 0.0)

            # Apply doc_type filter if requested
            if request.doc_type and request.doc_type != "all":
                hits = [h for h in hits if h.get("doc_type", "").lower() == request.doc_type.lower()]
                total_hits = len(hits)

            # Pagination
            paginated = hits[offset: offset + limit]

            proto_hits = []
            for h in paginated:
                proto_hits.append(pb2.SearchHit(
                    doc_id=h.get("doc_id", 0),
                    title=h.get("title", ""),
                    source=h.get("source", ""),
                    date=h.get("date", ""),
                    doc_type=h.get("doc_type", "policy"),
                    score=float(h.get("score", 0.0)),
                    length=h.get("length", 0),
                    snippet=h.get("snippet", ""),
                    matched_terms=h.get("matched_terms", [])
                ))

            total_elapsed_ms = (time.perf_counter() - t0) * 1000.0

            return pb2.SearchResponse(
                query=query,
                algorithm=algo_str,
                total_hits=total_hits,
                engine_latency_ms=round(eng_latency, 3),
                from_cache=from_cache,
                hits=proto_hits,
                error_message=""
            )
        except Exception as e:
            logger.error(f"Search query error: {e}")
            return pb2.SearchResponse(
                query=query,
                algorithm=algo_str,
                total_hits=0,
                engine_latency_ms=0.0,
                from_cache=False,
                hits=[],
                error_message=str(e)
            )

    def GetStats(self, request, context):
        try:
            stats = self.engine.get_stats()
            return pb2.StatsResponse(
                total_documents=stats.get("total_documents", 0),
                active_documents=stats.get("active_documents", 0),
                deleted_documents=stats.get("deleted_documents", 0),
                total_unique_terms=stats.get("total_unique_terms", 0),
                total_postings=stats.get("total_postings", 0),
                total_token_occurrences=stats.get("total_token_occurrences", 0),
                avg_doc_length=float(stats.get("avg_doc_length", 0.0)),
                estimated_memory_bytes=stats.get("estimated_memory_bytes", 0),
                cache_entries=stats.get("cache_entries", 0),
                cache_hits=stats.get("cache_hits", 0),
                cache_misses=stats.get("cache_misses", 0),
                engine_version="IndexForge C++20 Core v2.0"
            )
        except Exception as e:
            logger.error(f"Error in GetStats: {e}")
            return pb2.StatsResponse()

    def CheckHealth(self, request, context):
        uptime = int(time.time() - self.start_time)
        return pb2.HealthResponse(
            status="SERVING",
            engine_info="IndexForge C++20 Inverted Index Core (gRPC/HTTP2)",
            uptime_seconds=uptime
        )

    def SaveIndex(self, request, context):
        path = request.filepath or "index.forge"
        ok = self.engine.save_index(path)
        return pb2.SaveIndexResponse(
            success=ok,
            filepath=path,
            error_message="" if ok else "Failed to save index binary"
        )

    def LoadIndex(self, request, context):
        path = request.filepath or "index.forge"
        ok = self.engine.load_index(path)
        stats = self.engine.get_stats()
        return pb2.LoadIndexResponse(
            success=ok,
            filepath=path,
            documents_loaded=stats.get("total_documents", 0) if ok else 0,
            error_message="" if ok else "Failed to load index binary"
        )

    def VerifyIntegrity(self, request, context):
        path = request.filepath or "index.forge"
        res = self.engine.verify_integrity(path)
        return pb2.VerifyIntegrityResponse(
            valid=res.get("valid", False),
            checksum=str(res.get("checksum", "")),
            bytes_checked=res.get("bytes_checked", 0),
            error_message=res.get("error_message", "")
        )

    def ClearCache(self, request, context):
        self.engine.clear_cache()
        return pb2.ClearCacheResponse(success=True)

def serve(port: int = 50051):
    engine = IndexForgeBinding()

    # Preload sample documents from storage/sample_docs or data/sample_docs if empty
    stats = engine.get_stats()
    if stats.get("total_documents", 0) == 0:
        sample_dirs = [REPO_ROOT / "storage" / "sample_docs", REPO_ROOT / "data" / "sample_docs"]
        for sdir in sample_dirs:
            if sdir.exists():
                doc_id = 1
                for fpath in sdir.glob("*.*"):
                    if fpath.suffix.lower() in (".txt", ".md"):
                        try:
                            content = fpath.read_text(encoding="utf-8")
                            title = fpath.stem.replace("_", " ").title()
                            doc_type = "policy"
                            if "tender" in fpath.stem:
                                doc_type = "tender"
                            elif "regulation" in fpath.stem or "rmf" in fpath.stem or "sp800" in fpath.stem:
                                doc_type = "regulation"
                            elif "notice" in fpath.stem:
                                doc_type = "notice"
                            elif "doc" in fpath.stem or "transit" in fpath.stem or "strategy" in fpath.stem:
                                doc_type = "tech_doc"

                            engine.index_document(
                                doc_id=doc_id,
                                title=title,
                                source="Federal Repository",
                                date="2026-06-01",
                                doc_type=doc_type,
                                content=content
                            )
                            doc_id += 1
                        except Exception as e:
                            logger.warning(f"Error loading {fpath.name}: {e}")
                logger.info(f"Loaded {doc_id - 1} documents into C++ engine core.")
                break

    server = grpc.server(
        futures.ThreadPoolExecutor(max_workers=10),
        options=[
            ('grpc.max_send_message_length', 64 * 1024 * 1024),
            ('grpc.max_receive_message_length', 64 * 1024 * 1024),
        ]
    )
    pb2_grpc.add_SearchEngineServiceServicer_to_server(
        SearchEngineServiceImpl(engine), server
    )

    listen_addr = f"0.0.0.0:{port}"
    server.add_insecure_port(listen_addr)
    server.start()
    logger.info(f"================================================================")
    logger.info(f"  IndexForge C++20 gRPC Engine Server listening on {listen_addr}")
    logger.info(f"================================================================")
    server.wait_for_termination()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="IndexForge C++20 gRPC Server")
    parser.add_argument("--port", type=int, default=int(os.environ.get("GRPC_PORT", 50051)), help="gRPC server port (default: 50051)")
    args = parser.parse_args()
    serve(args.port)
