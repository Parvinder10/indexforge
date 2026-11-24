# IndexForge — Hybrid C++20 & Java 21 Enterprise Search Platform

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Java 21](https://img.shields.io/badge/Java-21-ED8B00?logo=openjdk)](https://openjdk.org/projects/jdk/21/)
[![Spring Boot 3](https://img.shields.io/badge/Spring_Boot-3.3.5-6DB33F?logo=springboot)](https://spring.io/projects/spring-boot)
[![gRPC](https://img.shields.io/badge/gRPC-HTTP%2F2-244c5a?logo=grpc)](https://grpc.io/)
[![React 18](https://img.shields.io/badge/React-18-61DAFB?logo=react)](https://react.dev/)
[![SQLite](https://img.shields.io/badge/SQLite-WAL_Mode-003B57?logo=sqlite)](https://sqlite.org/)
[![Live Interface](https://img.shields.io/badge/Live_Demo-GitHub_Pages-22c55e?logo=github)](https://parvinder10.github.io/indexforge/)

IndexForge is an enterprise hybrid search platform combining a **custom-built C++20 inverted index search core** with a **Java 21 + Spring Boot 3 application backend**, communicating over high-throughput **gRPC and Protocol Buffers**, and delivered through a responsive **React 18 + TypeScript web interface**.

---

## 🏗️ Architecture Overview

IndexForge establishes a clear architectural separation between **algorithmic computation** and **application workflow orchestration**:

```
+-------------------------------------------------------------------------+
|                       React 18 + TypeScript UI                         |
|                 (http://localhost:5174 or Docker:3000)                  |
+------------------------------------+------------------------------------+
                                     | HTTP/REST & JSON
                                     v
+-------------------------------------------------------------------------+
|                  Java 21 + Spring Boot 3 Application Layer              |
|                             (Port 8080)                                 |
|  - Document Ingestion & Validation    - Search Query Orchestrator       |
|  - Search History Audit Store         - OpenAPI 3.0 Documentation       |
|  - SQLite Metadata DB (WAL Mode)      - Exception & Timeout Handling    |
+------------------------------------+------------------------------------+
                                     | gRPC / Protobuf (HTTP/2 Binary)
                                     v
+-------------------------------------------------------------------------+
|                     C++20 High-Performance Engine Core                  |
|                             (Port 50051)                                |
|  - Inverted Index & Tokenizer         - Porter Stemmer & Stopwords      |
|  - Delta-VByte Postings Compression   - Okapi BM25 & TF-IDF Scoring     |
|  - Boolean AST Query Engine           - Binary Persistence with CRC32   |
+-------------------------------------------------------------------------+
```

### Architectural Division of Responsibilities

| Component | Responsibility | Technologies |
| :--- | :--- | :--- |
| **Search Engine Core** | Inverted index, tokenization, Porter stemming, Delta-VByte compression, posting list retrieval, BM25 & TF-IDF ranking, boolean AST evaluation, CRC32 persistence. | **C++20**, SIMD vectorization, `std::shared_mutex` |
| **Application Backend** | REST endpoints, metadata lifecycle, search history auditing, search filters/pagination, OpenAPI 3.0 docs, security/CORS, gRPC channel management. | **Java 21**, **Spring Boot 3.3.5**, **SQLite JDBC** |
| **Inter-Service IPC** | Binary contract definition, streaming and RPC calls, deadline propagation, retry policies, error mapping. | **gRPC 1.68**, **Protocol Buffers 3.25** |
| **Web Interface** | Real-time search query execution, BM25 hyperparameter tuning ($k_1, b$), search history audit modal, document viewer, index telemetry gauges. | **React 18**, **TypeScript**, **Tailwind CSS**, **Vite** |

---

## ⚡ Quick Start (Local Development)

### Prerequisites
- **C++ Compiler:** Clang++ (v14+), GCC (v11+), or MSVC (2022) with C++20 support
- **Java:** JDK 21 (or uses portable OpenJDK in `tools/jdk-21`)
- **Maven:** Apache Maven 3.9+ (or uses portable Maven in `tools/maven`)
- **Node.js:** Node.js v18+ & npm
- **Python:** Python 3.10+ (for gRPC daemon and launcher)

### 1-Command Startup
To launch the entire hybrid stack (C++ gRPC Core + Spring Boot Backend + React Frontend):

```bash
python run.py
```

The launcher will verify C++ compilation, start the gRPC core on `50051`, launch Spring Boot on `8080`, start Vite on `5174`, and open your browser automatically.

### Service Endpoints
- **React Frontend:** `http://localhost:5174`
- **Spring Boot REST API:** `http://127.0.0.1:8080`
- **Interactive Swagger / OpenAPI UI:** `http://127.0.0.1:8080/swagger-ui/index.html`
- **C++ Search Engine gRPC:** `127.0.0.1:50051`

---

## 🐳 Docker Deployment (Multi-Container)

To run the complete platform in isolated containers using Docker Compose:

```bash
docker compose -f docker/docker-compose.yml up --build
```

Services:
1. `indexforge-cpp-engine`: Multi-stage C++20 build and gRPC service on port `50051`.
2. `indexforge-java-api`: Eclipse Temurin Java 21 Spring Boot runtime on port `8080`.
3. `indexforge-frontend`: Nginx reverse proxy and React SPA on port `3000`.

Interface available at: **`http://localhost:3000`**

---

## 🧪 Testing & Verification

### 1. C++ Core Tests (GoogleTest / 23 Tests)
Verifies tokenization, Porter stemming, Delta-VByte compression, boolean AST queries, BM25 scoring, and CRC32 persistence:
```bash
./build/indexforge_tests.exe
```
Output:
```
=========================================================
Running 23 IndexForge Unit & Integration Tests
=========================================================
[       OK ] Concurrency - Multi-threaded concurrent read-only queries
[       OK ] Persistence - Save, verify, and reload index
[       OK ] RetrievalEngine - Phrase matching with token position verification
[       OK ] Ranking - BM25 mathematical properties and parameter influence
...
Test Summary: 23 passed, 0 failed in 429 ms
```

### 2. Java Spring Boot Tests (JUnit 5 & MockMvc / 9 Tests)
Verifies REST controllers, parameter validation, search history audit logging, and gRPC status mapping:
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

## 📊 Empirical Benchmarks (C++ Core vs Linear Scan)

Tested on x86_64 running Clang++ 23.1 with `-O3 -march=native`:

| Metric | 1,000 Documents | 10,000 Documents |
| :--- | :--- | :--- |
| **Document Indexing Speed** | **7,730 docs/sec** (129 ms) | **8,402 docs/sec** (1.19 s) |
| **Total Postings Managed** | 28,840 | 287,489 |
| **In-Memory Size** | 2.53 MB | 31.0 MB (~3.1 KB/doc) |
| **Compressed Index on Disk** | 1.13 MB (Delta-VByte) | 11.2 MB |
| **Linear Scan Mean Latency** | 92.77 ms | 1,005.70 ms |
| **Inverted Index BM25 Latency**| **1.15 ms** | **17.29 ms** |
| **Inverted Index p99 Latency** | **2.31 ms** | **34.11 ms** |
| **Speedup vs Linear Scan** | **80.6x FASTER** | **58.2x FASTER** |
| **Multi-Threaded Throughput** | **2,664.5 QPS** | **216.8 QPS** |

---

## 📖 In-Depth Documentation
- **[Architecture & Trade-Offs](docs/ARCHITECTURE.md)**: Deep dive into C++ cache locality, zero-GC tail latency, Delta-VByte encoding, Spring Boot orchestration, gRPC resiliency, and recovery protocols.
- **[Demonstration Guide](docs/DEMO.md)**: Step-by-step walkthrough covering search ranking, hyperparameter tuning, audit history inspection, live document ingestion, and Swagger API testing.

---

## 📄 License
Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.
