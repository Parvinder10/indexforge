# IndexForge — Step-by-Step Live Demonstration Guide

This guide walks you through verifying and demonstrating the complete **IndexForge Hybrid C++20 & Java 21 Search Platform** running locally on your workstation.

---

## 1. Quick Launch (One-Command Startup)

Open your terminal in the repository root and run:

```bash
python run.py
```

This single command automatically:
1. Detects or compiles the high-performance **C++20 search engine library** (`build/indexforge.dll` or `libindexforge.so`).
2. Starts the **C++ Search Engine gRPC Server** listening on port `50051`.
3. Starts the **Java 21 Spring Boot 3 Application** on port `8080`.
4. Starts the **React 18 + TypeScript Vite Development Server** on port `5174`.
5. Opens your default web browser directly to `http://localhost:5174`.

### Service Port Map
| Service | URL / Port | Purpose |
| :--- | :--- | :--- |
| **Frontend Web Interface** | `http://localhost:5174` | React 18 + TypeScript Search UI |
| **Spring Boot REST API** | `http://127.0.0.1:8080` | Document Ingestion, Metadata & Search API |
| **OpenAPI / Swagger UI** | `http://127.0.0.1:8080/swagger-ui/index.html` | Interactive REST documentation & testing |
| **C++ Search Engine Core** | `127.0.0.1:50051` | High-Performance gRPC Inverted Index Core |

---

## 2. Demonstration Scenarios

### Scenario A: Full-Text Search with Okapi BM25 Ranking
1. In the search input bar, type `public tender`.
2. Notice the instant sub-millisecond query latency (`< 1.0 ms`).
3. Observe the ranked results:
   - Rank 1: **Nasa Artemis Procurement Tender** (Relevance: `~2.056`)
   - Rank 2: **Epa Clean Energy Standard** (Relevance: `~1.830`)
   - Rank 3: **Gsa It Modernization Tender** (Relevance: `~1.763`)
4. Notice the snippet text with `<mark>` highlighted matching terms.
5. Inspect the relevance score gauges and token position metrics.

---

### Scenario B: Algorithm Comparison (BM25 vs TF-IDF vs Linear Scan)
1. In the algorithm dropdown, switch from **Okapi BM25** to **TF-IDF**.
   - Notice how TF-IDF recalibrates scores based on inverse document frequency without asymptotic term saturation.
2. Adjust the BM25 hyperparameters in the UI:
   - **$k_1$ parameter** (Term Frequency Saturation): Slide between `0.5` and `3.0`.
   - **$b$ parameter** (Document Length Normalization): Slide between `0.0` and `1.0`.
3. Toggle the **"Linear Scan (Unindexed Baseline)"** button:
   - Observe the latency comparison between the compressed C++ inverted index (`< 1 ms`) versus the unindexed brute-force sequential file scan (`10x–100x slower`).

---

### Scenario C: Search Query History Audit Log
1. Click the **"📜 History"** button in the top navigation bar.
2. The modal displays queries logged in real time into **SQLite** by the Java Spring Boot service:
   - Query text (e.g., `public tender`, `cybersecurity`)
   - Ranking algorithm used (`bm25`)
   - Total hits returned
   - End-to-end execution latency in milliseconds
   - Exact ISO timestamp
3. Click any past query in the list to re-execute it instantly.
4. Click **"Clear History"** to test audit record purging.

---

### Scenario D: Live Document Ingestion into C++ Core
1. Click the **"+ Ingest"** button in the top navigation bar.
2. Select **"Text Input"** tab and fill in:
   - **Title**: `Quantum Key Distribution Protocol`
   - **Source**: `National Research Lab`
   - **Doc Type**: `tech_doc`
   - **Content**: `Quantum entanglement-based photon key exchange protocol providing tamper-evident cryptographic security across metropolitan fiber networks.`
3. Click **"Ingest Document"**.
4. Observe the confirmation response from Spring Boot and C++:
   - Unique integer `doc_id` assigned by SQLite.
   - Exact token count indexed into C++ posting lists.
5. In the main search bar, search for `quantum photon`:
   - Your newly ingested document appears immediately at Rank 1 with sub-millisecond latency!

---

### Scenario E: Inspecting C++ Index Statistics & Telemetry
1. Click the **"📊 Stats"** button in the top navigation bar.
2. The modal displays real-time telemetry gathered from the C++ core via gRPC:
   - **Total Active Documents**: 12+ documents
   - **Unique Vocabulary Terms**: ~850 terms
   - **Total Postings Count**: ~1,350 posting records
   - **Delta-VByte Memory Footprint**: ~136 KB
   - **Query Cache Hit/Miss Counters**
   - **C++ Engine Version & Architecture**
3. Click **"Verify Integrity (CRC32)"** to demonstrate live binary checksum verification.

---

### Scenario F: Interactive OpenAPI / Swagger UI
1. Open `http://127.0.0.1:8080/swagger-ui/index.html` in your browser.
2. Explore the documented REST API endpoints:
   - `GET /api/search`: Query C++ engine with BM25 / TF-IDF
   - `GET /api/search/history`: Retrieve search query audit trail
   - `GET /api/documents`: Paginated document listing from SQLite
   - `POST /api/ingest/text`: Ingest document into SQLite + C++ Core
   - `GET /api/index/stats`: Retrieve index telemetry
   - `GET /api/index/health`: Retrieve gRPC engine health status
3. Execute any endpoint directly through Swagger's **"Try it out"** interface.

---

## 3. Running Automated Test Suites

### Run All C++20 Core Unit Tests (GoogleTest)
```bash
./build/indexforge_tests.exe
```
Output:
```
=========================================================
Running 23 IndexForge Unit & Integration Tests
=========================================================
[       OK ] Concurrency - Multi-threaded concurrent read-only queries
[       OK ] Concurrency - Concurrent search during incremental ingestion
[       OK ] Persistence - Save, verify, and reload index
[       OK ] Persistence - Corrupted and invalid file detection
[       OK ] QueryParser - Boolean expressions AST generation
[       OK ] RetrievalEngine - Phrase matching with token position verification
[       OK ] RetrievalEngine - Boolean AND, OR, NOT execution
[       OK ] Ranking - BM25 mathematical properties and parameter influence
[       OK ] Ranking - TF-IDF calculation correctness
[       OK ] Compression - Postings list round trip with delta encoding
...
Test Summary: 23 passed, 0 failed in 429 ms
```

### Run All Java Spring Boot Tests (JUnit 5 & MockMvc)
```bash
cd java-api
mvn test
```
Output:
```
[INFO] Running com.indexforge.controller.SearchControllerTest
[INFO] Tests run: 3, Failures: 0, Errors: 0, Skipped: 0
[INFO] Running com.indexforge.controller.DocumentControllerTest
[INFO] Tests run: 4, Failures: 0, Errors: 0, Skipped: 0
[INFO] Running com.indexforge.controller.IndexControllerTest
[INFO] Tests run: 2, Failures: 0, Errors: 0, Skipped: 0
[INFO] Results: Tests run: 9, Failures: 0, Errors: 0, Skipped: 0
[INFO] BUILD SUCCESS
```

---

## 4. Multi-Container Docker Deployment

To run the complete hybrid platform inside Docker:

```bash
docker compose -f docker/docker-compose.yml up --build
```

This spins up:
1. `indexforge-cpp-engine`: C++20 gRPC engine on port `50051`.
2. `indexforge-java-api`: Java 21 Spring Boot 3 API on port `8080`.
3. `indexforge-frontend`: Nginx serving React 18 UI on port `3000`.

Access the interface at `http://localhost:3000`.
