import os
import time
import shutil
from pathlib import Path
from typing import Optional, List, Dict, Any
from fastapi import FastAPI, Query, HTTPException
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field

from .indexforge_binding import IndexForgeBinding
from .metadata_db import MetadataDB
from .ingestion import DocumentIngestionPipeline

app = FastAPI(
    title="IndexForge — High-Performance Full-Text Search Engine API",
    description="C++20 Inverted Index Core with BM25 / TF-IDF Ranking, SQLite Metadata, and High-Throughput Retrieval",
    version="1.0.0"
)

# Enable CORS for React frontend (Vite dev server and production)
app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:5173",
        "http://127.0.0.1:5173",
        "http://localhost:3000",
        "http://127.0.0.1:3000",
        "http://localhost:8000",
        "http://127.0.0.1:8000",
    ],
    allow_origin_regex=r"^https?://(localhost|127\.0\.0\.1)(:\d+)?$",
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Mount compiled frontend assets if available
frontend_dist = Path(__file__).resolve().parent.parent / "frontend" / "dist"
if (frontend_dist / "assets").exists():
    app.mount("/assets", StaticFiles(directory=str(frontend_dist / "assets")), name="assets")

# Global engine and DB instances
ENGINE: Optional[IndexForgeBinding] = None
DB: Optional[MetadataDB] = None
PIPELINE: Optional[DocumentIngestionPipeline] = None

@app.get("/", response_class=HTMLResponse)
def serve_frontend():
    dist_index = frontend_dist / "index.html"
    if dist_index.exists():
        with open(dist_index, "r", encoding="utf-8") as f:
            return f.read()
    html_file = Path(__file__).resolve().parent / "static" / "index.html"
    if html_file.exists():
        with open(html_file, "r", encoding="utf-8") as f:
            return f.read()
    return "<h1>IndexForge API running. Visit /docs for OpenAPI documentation.</h1>"

@app.on_event("startup")
def startup_event():
    global ENGINE, DB, PIPELINE
    try:
        ENGINE = IndexForgeBinding()
        DB = MetadataDB()
        PIPELINE = DocumentIngestionPipeline(ENGINE, DB)

        # 1. Try loading persisted C++ index if available
        forge_file = Path(__file__).resolve().parent.parent / "index.forge"
        if forge_file.exists():
            ENGINE.load_index(str(forge_file))

        # 2. If C++ engine is empty, re-populate from DB or sample docs
        engine_stats = ENGINE.get_stats()
        if engine_stats.get("total_documents", 0) == 0:
            db_docs = DB.list_documents(limit=10000)
            if db_docs:
                print(f"Re-indexing {len(db_docs)} documents from SQLite metadata database...")
                for d in db_docs:
                    full_doc = DB.get_document(d["id"])
                    if full_doc and full_doc.get("content"):
                        ENGINE.index_document(
                            doc_id=d["id"],
                            title=full_doc.get("title", ""),
                            source=full_doc.get("source", ""),
                            date=full_doc.get("date", ""),
                            doc_type=full_doc.get("doc_type", "policy"),
                            content=full_doc.get("content", "")
                        )
                print("Re-indexing complete.")
            else:
                sample_dir = Path(__file__).resolve().parent.parent / "data" / "sample_docs"
                if sample_dir.exists():
                    print(f"Indexing initial sample documents from {sample_dir}...")
                    PIPELINE.ingest_directory(str(sample_dir))
                    print(f"Indexed {DB.count_documents()} sample documents.")

        # Persist index snapshot
        ENGINE.save_index(str(forge_file))
    except Exception as e:
        print(f"Warning during startup: {e}")

class IngestTextRequest(BaseModel):
    title: str = Field(..., example="Federal Cloud Migration Plan")
    source: str = Field(default="Government Repository", example="OMB")
    date: str = Field(default="2026-01-01", example="2026-03-15")
    doc_type: str = Field(default="policy", example="tender")
    content: str = Field(..., example="Full text content of the document...")

class IngestFileRequest(BaseModel):
    file_name: str
    content: str
    title: Optional[str] = None
    source: Optional[str] = None
    date: Optional[str] = None
    doc_type: Optional[str] = None

class BenchmarkRequest(BaseModel):
    corpus_size: int = Field(default=1000, example=1000)

@app.get("/api/health")
def get_health():
    return {
        "status": "healthy",
        "engine": "IndexForge C++20 Core",
        "bindings": "ctypes C-ABI",
        "timestamp": time.time()
    }

@app.get("/api/stats")
def get_stats():
    if not ENGINE:
        raise HTTPException(status_code=503, detail="Search engine not initialized")
    engine_stats = ENGINE.get_stats()
    db_count = DB.count_documents() if DB else 0
    doc_types = DB.get_doc_types() if DB else []

    return {
        **engine_stats,
        "sqlite_documents": db_count,
        "available_doc_types": doc_types
    }

@app.get("/api/search")
def search(
    q: str = Query(..., description="Query string, keywords, boolean expression, or exact \"phrase\""),
    algo: str = Query(default="bm25", pattern="^(bm25|tfidf)$"),
    k1: float = Query(default=1.2, ge=0.0, le=5.0),
    b: float = Query(default=0.75, ge=0.0, le=1.0),
    doc_type: Optional[str] = Query(default=None),
    limit: int = Query(default=20, ge=1, le=100),
    offset: int = Query(default=0, ge=0)
):
    if not ENGINE:
        raise HTTPException(status_code=503, detail="Search engine not initialized")

    t0 = time.perf_counter()
    res = ENGINE.search(query=q, algo=algo, k1=k1, b=b, limit=limit * 2, offset=0)
    hits = res.get("hits", [])

    # Apply doc_type filter if specified
    if doc_type and doc_type != "all":
        hits = [h for h in hits if h.get("doc_type", "").lower() == doc_type.lower()]

    total_filtered = len(hits)
    paginated_hits = hits[offset: offset + limit]

    total_latency_ms = (time.perf_counter() - t0) * 1000.0

    return {
        "query": q,
        "algorithm": algo,
        "k1": k1,
        "b": b,
        "total_hits": total_filtered,
        "engine_latency_ms": res.get("latency_ms", 0.0),
        "total_latency_ms": round(total_latency_ms, 3),
        "from_cache": res.get("from_cache", False),
        "hits": paginated_hits
    }

@app.get("/api/search/linear")
def search_linear(
    q: str = Query(..., description="Query string for baseline linear scan"),
    limit: int = Query(default=20, ge=1, le=100)
):
    if not ENGINE:
        raise HTTPException(status_code=503, detail="Search engine not initialized")

    t0 = time.perf_counter()
    res = ENGINE.linear_search(query=q, limit=limit)
    total_latency_ms = (time.perf_counter() - t0) * 1000.0

    return {
        "query": q,
        "method": "Linear Document Scan Baseline",
        "total_hits": res.get("total_hits", 0),
        "engine_latency_ms": res.get("latency_ms", 0.0),
        "total_latency_ms": round(total_latency_ms, 3),
        "hits": res.get("hits", [])
    }

@app.get("/api/documents")
def list_documents(
    doc_type: Optional[str] = None,
    limit: int = Query(default=20, ge=1, le=100),
    offset: int = Query(default=0, ge=0)
):
    if not DB:
        raise HTTPException(status_code=503, detail="Database not initialized")
    docs = DB.list_documents(doc_type=doc_type, limit=limit, offset=offset)
    total = DB.count_documents(doc_type=doc_type)
    return {
        "total": total,
        "limit": limit,
        "offset": offset,
        "documents": docs
    }

@app.get("/api/documents/{doc_id}")
def get_document(doc_id: int):
    if not DB:
        raise HTTPException(status_code=503, detail="Database not initialized")
    doc = DB.get_document(doc_id)
    if not doc:
        raise HTTPException(status_code=404, detail="Document not found")
    return doc

@app.delete("/api/documents/{doc_id}")
def delete_document(doc_id: int):
    if not ENGINE or not DB:
        raise HTTPException(status_code=503, detail="Service not initialized")
    
    eng_ok = ENGINE.delete_document(doc_id)
    db_ok = DB.delete_document(doc_id)

    return {
        "success": eng_ok or db_ok,
        "doc_id": doc_id,
        "engine_deleted": eng_ok,
        "db_deleted": db_ok
    }

@app.post("/api/ingest/text")
def ingest_text(req: IngestTextRequest):
    if not PIPELINE:
        raise HTTPException(status_code=503, detail="Pipeline not initialized")

    res = PIPELINE.ingest_single(
        content=req.content,
        title=req.title,
        source=req.source,
        date=req.date,
        doc_type=req.doc_type
    )
    return res

@app.post("/api/ingest/file")
def ingest_file(req: IngestFileRequest):
    if not PIPELINE:
        raise HTTPException(status_code=503, detail="Pipeline not initialized")

    res = PIPELINE.ingest_single(
        content=req.content,
        title=req.title,
        source=req.source,
        date=req.date,
        doc_type=req.doc_type,
        filename=req.file_name
    )
    return res

@app.post("/api/ingest/sample")
def ingest_sample_documents():
    if not PIPELINE:
        raise HTTPException(status_code=503, detail="Pipeline not initialized")
    sample_dir = Path(__file__).resolve().parent.parent / "data" / "sample_docs"
    results = PIPELINE.ingest_directory(str(sample_dir))
    return {
        "status": "success",
        "ingested_count": len(results),
        "documents": results
    }

@app.post("/api/cache/clear")
def clear_cache():
    if ENGINE:
        ENGINE.clear_cache()
    return {"status": "cache cleared"}

@app.post("/api/index/save")
def save_index(filepath: str = "index.forge"):
    if not ENGINE:
        raise HTTPException(status_code=503, detail="Engine not initialized")
    ok = ENGINE.save_index(filepath)
    return {"status": "saved" if ok else "failed", "filepath": filepath}

@app.post("/api/index/load")
def load_index(filepath: str = "index.forge"):
    if not ENGINE:
        raise HTTPException(status_code=503, detail="Engine not initialized")
    ok = ENGINE.load_index(filepath)
    return {"status": "loaded" if ok else "failed", "filepath": filepath}

@app.post("/api/index/verify")
def verify_index(filepath: str = "index.forge"):
    if not ENGINE:
        raise HTTPException(status_code=503, detail="Engine not initialized")
    res = ENGINE.verify_integrity(filepath)
    return res

@app.post("/api/benchmark")
def run_benchmark(req: BenchmarkRequest):
    """Run an on-demand comparative benchmark between Inverted Index (BM25 & TF-IDF) and Linear Scan"""
    import subprocess
    benchmark_bin = Path(__file__).resolve().parent.parent / "build" / "indexforge_benchmark.exe"
    if not benchmark_bin.exists():
        raise HTTPException(status_code=404, detail="Benchmark binary not found")

    size = min(req.corpus_size, 10000) # Safety cap for interactive API calls
    proc = subprocess.run([str(benchmark_bin), str(size)], capture_output=True, text=True, timeout=60)
    return {
        "corpus_size": size,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "return_code": proc.returncode
    }

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("api.main:app", host="127.0.0.1", port=8000, reload=True)
