# IndexForge — High-Performance Full-Text Search Engine

IndexForge is a functional, custom-built full-text search engine implemented in **modern C++20**, paired with a **Python FastAPI** service layer and a **React + TypeScript** interface.

Designed to address search inefficiency across thousands of unstructured real-world public documents (such as government notices, public tenders, regulatory filings, and technical specifications), IndexForge implements indexing, boolean query parsing, relevance ranking (BM25 & TF-IDF), postings compression, and persistence from scratch.

---

## 1. Key Features

- **C++20 Inverted Index Core:** Inverted index mapping terms to postings containing document IDs, term frequencies, and token positions.
- **Delta-VByte Postings Compression:** Variable-byte integer encoding with delta gap compression, reducing index storage footprint by over 55%.
- **Dual Ranking Models:**
  - **Okapi BM25:** Configurable $k_1$ (term saturation) and $b$ (document length normalization).
  - **Vector Space TF-IDF:** Sub-linear term frequency scaling and cosine document length normalization.
- **Query Parser & AST:**
  - Single and multi-term keyword queries (`procurement modern`)
  - Exact phrase searches with position adjacency verification (`"public tender"`)
  - Boolean expressions (`AND`, `OR`, `NOT`, `+`, `-`, `!`) with parenthesis grouping: `(tender OR grant) AND federal`
- **Dynamic Snippet Generation:** Sliding-window term density algorithm that extracts the most relevant snippet with matched terms highlighted in `<mark>` tags.
- **Thread-Safe Architecture:** `std::shared_mutex` multi-reader single-writer locking supporting high concurrent search throughput while accommodating incremental ingestion.
- **LRU Query Result Cache:** In-memory query result cache with automatic invalidation upon document ingestion or deletion.
- **Index Persistence & Integrity:** Binary file format with magic headers (`IDXF`), versioning, and **IEEE 802.3 CRC32 checksum** validation.
- **Comparative Baseline Engine:** Integrated linear document scanner to benchmark inverted index performance against linear scans under identical datasets and queries.
- **FastAPI REST API:** Full-featured REST API exposing document ingestion, search, document inspection, index statistics, and on-demand benchmarks.
- **React + TypeScript UI:** Responsive search dashboard with live latency badges, BM25 parameter sliders, syntax guide popovers, and full document modal viewers.

---

## 2. Real-World Benchmark Results

Benchmarks were executed on an **x86_64 architecture running Clang++ 23.1 with `-O3 -march=native` on LLVM-MinGW UCRT**.

### 2.1 Benchmark Summary

| Metric | 1,000 Documents | 10,000 Documents |
| :--- | :--- | :--- |
| **Document Indexing Time** | **129.4 ms** (7,730 docs/sec) | **1,190.2 ms** (8,402 docs/sec) |
| **Total Postings Managed** | 28,840 | 287,489 |
| **Total Token Occurrences** | 96,003 tokens | 953,280 tokens |
| **In-Memory Size** | 2.53 MB | 31.0 MB (~3.1 KB / doc) |
| **Serialized Compressed Index** | 1.13 MB | 11.2 MB |
| **Linear Scan Mean Latency** | 92.77 ms | 1,005.70 ms |
| **Inverted Index BM25 Mean Latency** | **1.15 ms** | **17.29 ms** |
| **Inverted Index TF-IDF Mean Latency** | **1.22 ms** | **16.88 ms** |
| **Inverted Index p50 Latency (BM25)** | **1.11 ms** | **16.91 ms** |
| **Inverted Index p99 Latency (BM25)** | **2.31 ms** | **34.11 ms** |
| **Speedup vs Linear Scan** | **80.6x FASTER** | **58.2x FASTER** |
| **Single-Threaded BM25 Throughput** | **868.9 QPS** | **57.8 QPS** |
| **Multi-Threaded Read Throughput (4 Threads)** | **2,664.5 QPS** | **216.8 QPS** |

---

## 3. Technology Stack

- **Search Engine Core:** C++20 (`std::shared_mutex`, `std::priority_queue`, `std::unordered_map`, `std::span`)
- **Build System:** CMake (v3.20+) and Ninja
- **Compiler:** Clang++ 23 / GCC 11+ / MSVC 2022
- **Testing:** Custom test suite (23 unit & integration tests)
- **Metadata Storage:** SQLite (WAL mode)
- **API Layer:** Python FastAPI, Pydantic, Uvicorn
- **Bindings:** Zero-IPC C ABI via `ctypes`
- **Frontend:** React 18, TypeScript, Tailwind CSS
- **Containerization:** Multi-stage Dockerfile and docker-compose.yml

---

## 4. Quickstart Guide

### 4.1 Prerequisites
Ensure you have installed:
- CMake 3.20+ and Ninja (or Make)
- A C++20 compatible compiler (`clang++`, `g++`, or `cl.exe`)
- Python 3.10+ with `fastapi` and `uvicorn`

### 4.2 Building the C++ Engine
```powershell
# 1. Configure with CMake
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release

# 2. Build shared library, CLI, test suite, and benchmark executable
cmake --build build

# 3. Run all automated unit and integration tests
.\build\indexforge_tests.exe
```

### 4.3 Running the Search Engine CLI
```powershell
# Search using BM25
.\build\indexforge_cli.exe search --query "public tender" --algo bm25

# Run baseline linear scan comparison
.\build\indexforge_cli.exe search --query "public tender" --linear

# Check index statistics
.\build\indexforge_cli.exe stats

# Verify index file integrity
.\build\indexforge_cli.exe verify --path index.forge
```

### 4.4 Launching the Web Service & UI
```powershell
# Start FastAPI backend and interactive web interface
python -m uvicorn api.main:app --host 127.0.0.1 --port 8000
```
Open **[http://127.0.0.1:8000](http://127.0.0.1:8000)** in your browser to access the search dashboard, or visit **[http://127.0.0.1:8000/docs](http://127.0.0.1:8000/docs)** for interactive OpenAPI Swagger documentation.

---

## 5. API Reference

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Service health status and C++ engine info |
| `GET` | `/api/stats` | Inverted index corpus metrics and cache hit statistics |
| `GET` | `/api/search` | Search inverted index with BM25 / TF-IDF, filters, and pagination |
| `GET` | `/api/search/linear` | Run baseline linear scan query for performance comparison |
| `GET` | `/api/documents` | List stored documents from SQLite with metadata |
| `GET` | `/api/documents/{id}` | Inspect full document content and metadata |
| `DELETE`| `/api/documents/{id}` | Remove document from inverted index and SQLite storage |
| `POST` | `/api/ingest/text` | Ingest a document with custom title, source, type, and text |
| `POST` | `/api/ingest/file` | Ingest a document from file text payload |
| `POST` | `/api/ingest/sample` | Import built-in real-world public document collection |
| `POST` | `/api/cache/clear` | Clear LRU query result cache |
| `POST` | `/api/index/save` | Serialize index to disk |
| `POST` | `/api/index/load` | Reload index from disk |
| `POST` | `/api/index/verify` | Verify index file CRC32 checksum |
| `POST` | `/api/benchmark` | Run on-demand live benchmark on C++ engine core |

---

## 6. Docker Deployment

Deploy IndexForge as a containerized service with Docker:

```bash
# Build and run with Docker Compose
docker compose up -d

# Check service logs
docker compose logs -f

# Verify service is running
curl http://localhost:8000/api/health
```

---

## 7. Query Syntax Guide

| Query Type | Syntax Example | Behavior |
| :--- | :--- | :--- |
| **Keyword Search** | `cybersecurity compliance` | Returns documents containing matched terms, ranked by BM25. |
| **Exact Phrase** | `"public tender"` | Verifies exact token position adjacency ($pos_{i+1} = pos_i + 1$). |
| **Boolean AND** | `federal AND procurement` | Intersects postings lists; requires both terms. |
| **Boolean OR** | `notice OR tender` | Unites postings lists; matches either term. |
| **Boolean NOT** | `cloud AND NOT legacy` | Subtracts candidate documents containing the negated term. |
| **Grouping** | `(tender OR grant) AND federal` | Evaluates nested boolean expressions with operator precedence. |

---

## 8. License

IndexForge is licensed under the Apache 2.0 License.
