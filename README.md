# IndexForge — High-Performance Full-Text Search Engine

[![GitHub License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![FastAPI](https://img.shields.io/badge/FastAPI-0.115+-009688?logo=fastapi)](https://fastapi.tiangolo.com/)
[![React](https://img.shields.io/badge/React-18-61DAFB?logo=react)](https://react.dev/)
[![Live Demo](https://img.shields.io/badge/Live_Demo-GitHub_Pages-22c55e?logo=github)](https://parvinder10.github.io/indexforge/)

IndexForge is a functional, custom-built full-text search engine implemented in **modern C++20**, paired with a **Python FastAPI** service layer and a **React + TypeScript** interface.

Designed to address search inefficiency across thousands of unstructured real-world public documents (such as government notices, public tenders, regulatory filings, and technical specifications), IndexForge implements indexing, boolean query parsing, relevance ranking (BM25 & TF-IDF), postings compression, and persistence from scratch.

---

## 🌐 Live Interface & Demos

You can explore and interact with IndexForge in multiple ways:

### 1. Online Live Web Demo (Zero Installation)
Experience IndexForge instantly in your browser powered by GitHub Pages:
- **Live Demo Link:** **[https://parvinder10.github.io/indexforge/](https://parvinder10.github.io/indexforge/)**
- **Features:** Client-side BM25 & TF-IDF relevance ranking, full-text Boolean and exact phrase search, interactive parameter sliders ($k_1, b$), document viewer modals, and real-time highlighted snippets across government notices, tenders, and policies.

> **GitHub Pages Configuration:**  
> If deploying a fork or fresh repository, enable GitHub Pages via:  
> `Repository Settings` → `Pages` → `Source: Deploy from a branch` → `Branch: main, folder: /docs` → `Save`.

### 2. Local 1-Click Desktop Launcher
Run the high-performance native C++ engine and web interface locally with a single command:
```bash
python run.py
```
This launcher automatically verifies the compiled C++ engine, starts the FastAPI service at `http://127.0.0.1:8000`, and opens the dashboard in your default browser.

### 3. Interactive API Documentation
- **Swagger UI:** `http://127.0.0.1:8000/docs`
- **ReDoc:** `http://127.0.0.1:8000/redoc`

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

## 4. Local Development & Interface Preview Guide

Follow this guide to run, inspect, and test the complete application locally on your machine.

### 4.1 Prerequisites
Ensure the following tools are installed:
- **C++20 Compiler:** Clang++ (v14+), GCC (v11+), or MSVC (v19.29+ / VS 2022)
- **Build System:** CMake (v3.20+) and Ninja (or Make)
- **Python:** Python 3.10+ (with `pip`)
- **Node.js:** Node.js v18+ and `npm` (for the React + Vite development server)

---

### 4.2 1-Click Full-Stack Startup (Recommended)

IndexForge includes an automated launcher that verifies your C++ build, checks Python and Node dependencies, starts the backend and frontend development servers concurrently, and launches your default browser:

```bash
# Windows / macOS / Linux
python run.py

# Or use the convenience scripts:
# Windows:
start.bat

# Linux / macOS:
./start.sh
```

#### What `run.py` does automatically:
1. **Verifies C++ Core:** Checks for `build/indexforge.dll` (or `.so`/`.dylib`); automatically runs CMake compilation if not found.
2. **Checks Dependencies:** Ensures `fastapi`, `uvicorn`, `pydantic`, and frontend `node_modules` are ready.
3. **Starts FastAPI Backend:** Spawns on `http://127.0.0.1:8000` with CORS configured for local dev ports.
4. **Starts Vite Dev Server:** Spawns on `http://localhost:5173` with Hot Module Replacement (HMR) and `/api` reverse proxying.
5. **Opens Your Browser:** Immediately navigates to `http://localhost:5173`.
6. **Graceful Teardown:** Pressing `Ctrl+C` terminates both services cleanly without leaving zombie processes.

#### CLI Options for `run.py`:
```bash
python run.py --backend-only      # Launch only FastAPI backend (port 8000)
python run.py --frontend-only     # Launch only Vite frontend (port 5173)
python run.py --build-only        # Compile C++ engine and build frontend dist, then exit
python run.py --backend-port 8080 # Custom backend port
python run.py --frontend-port 3000# Custom frontend port
python run.py --no-browser        # Do not launch default browser automatically
```

---

### 4.3 Running Services Individually (Manual Mode)

If you prefer running services in separate terminal windows:

#### Terminal 1: Build C++ Core and Start Backend API
```powershell
# 1. Compile C++ engine and run test suite
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\indexforge_tests.exe

# 2. Launch FastAPI with auto-reload
python -m uvicorn api.main:app --host 127.0.0.1 --port 8000 --reload
```
- **Backend API & UI:** [http://127.0.0.1:8000](http://127.0.0.1:8000)
- **Interactive OpenAPI Documentation:** [http://127.0.0.1:8000/docs](http://127.0.0.1:8000/docs)

#### Terminal 2: Start React + Vite Dev Server
```powershell
cd frontend
npm install
npm run dev
```
- **React Frontend (with HMR):** [http://localhost:5173](http://localhost:5173)

---

### 4.4 Local Testing & UI Walkthrough

Once running, you can test every interactive component with real system data:

1. **Keyword & Relevance Search:**
   - Type `public tender` or `cybersecurity compliance` and view ranked hits.
   - Switch between **BM25** and **TF-IDF** algorithms.
   - Adjust the **$k_1$** (term saturation) and **$b$** (length normalization) sliders to watch rankings update live.
2. **Boolean & Phrase Search:**
   - Test exact phrase matching: `"public tender"` (verifies token position adjacency).
   - Test Boolean logic: `federal AND procurement`, `notice OR tender`, `compliance NOT legacy`.
   - Test grouping: `(security OR cloud) AND federal`.
3. **Linear Scan Comparison:**
   - Toggle the **Linear Scan** switch to run queries through the unindexed baseline scanner and compare latency against the Inverted Index.
4. **Document Inspection & Deletion:**
   - Click any search result to open the full document in the **Document Viewer Modal**.
   - Inspect full text, source, date, token length, and file metadata.
   - Click **Delete Document** to test atomic deletion from both the Inverted Index and SQLite storage.
5. **Document Ingestion:**
   - Click **+ Ingest** to add a custom document with your own title, source, type, and content.
   - Or click **Load Built-in Real-World Dataset** to re-import standard federal documents.
6. **Corpus Statistics & Integrity Verification:**
   - Click **📊 Stats** to view active documents, unique terms, postings count, memory footprint, and cache hit rates.
   - Click **Verify CRC32** to run an IEEE 802.3 checksum validation on the binary index file.
   - Click **Clear Cache** or **Save/Load Index** to test persistence.
7. **Live Engine Benchmarking:**
   - Click **⚡ Benchmarks** to view empirical scaling tables (1k vs 10k documents).
   - Click **Run Live Benchmark** to execute `indexforge_benchmark.exe` directly on your CPU and inspect live throughput.

---

### 4.5 Troubleshooting Common Local Issues

| Issue | Cause | Solution |
| :--- | :--- | :--- |
| **Port 8000 or 5173 already in use** | Another local service is bound to the port. | Run `python run.py --backend-port 8080 --frontend-port 5174` |
| **Node.js or npm not installed** | Node is not present on the system. | Open `http://127.0.0.1:8000/`. FastAPI serves the precompiled production React build directly without requiring Node.js. |
| **CMake cannot find compiler** | Missing C++20 toolchain. | Ensure Clang, GCC, or Visual Studio C++ Build Tools (MSVC) are installed and accessible in your system `PATH`. |
| **Missing Python packages** | Virtualenv lacks dependencies. | Run `pip install fastapi uvicorn pydantic`. |
| **CORS errors in browser console** | Origin mismatch during manual startup. | CORS is configured for all local ports (`localhost` & `127.0.0.1` on any port) in `api/main.py`. Ensure you access via `localhost` or `127.0.0.1`. |

---

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
