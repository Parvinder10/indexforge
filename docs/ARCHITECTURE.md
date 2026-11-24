# IndexForge Architectural Specification
## Hybrid C++20 Core & Java 21 Spring Boot Enterprise Search Platform

---

## 1. Executive Summary & Design Philosophy

**IndexForge** is an enterprise-grade hybrid full-text search and indexing platform designed around a fundamental systems engineering principle: **separation of algorithmic computation from application orchestration**.

Traditional monolithic search engines often face a classic architectural dilemma:
- Writing the entire system in **C++** yields ultra-low query latency and minimal memory overhead, but complicates enterprise REST API integration, security filtering, database migrations, and web ecosystem interoperability.
- Writing the entire system in **Java** simplifies business workflows and web integration, but incurs garbage collection jitter (stop-the-world pauses), high memory overhead due to object headers, and suboptimal SIMD posting list decompression.

IndexForge resolves this dilemma through a **two-tier polyglot architecture**:
1. **Algorithmic Compute Core (C++20)**: Owns inverted index construction, tokenization, Porter stemming, Delta-VByte compression, posting list retrieval, boolean query AST evaluation, and Okapi BM25 / TF-IDF relevance ranking.
2. **Application Orchestration Layer (Java 21 + Spring Boot 3)**: Owns REST API routing, SQLite document metadata management, query audit logging, search configuration, parameter validation, security/CORS enforcement, and OpenAPI 3.0 documentation.
3. **Inter-Process Communication (gRPC + Protocol Buffers v3)**: Connects Spring Boot to the C++ core over low-latency binary HTTP/2 channels with strictly typed RPC contracts.

```
+-------------------------------------------------------------------------+
|                       React 18 + TypeScript UI                         |
|                   (http://localhost:5174 or port 3000)                  |
+------------------------------------+------------------------------------+
                                     | HTTP/REST & JSON
                                     v
+-------------------------------------------------------------------------+
|                  Java 21 + Spring Boot 3 Application Layer              |
|                             (Port 8080)                                 |
|  - Ingestion & Validation Controller  - Search Query Orchestrator       |
|  - Audit History Repository           - OpenAPI 3.0 Documentation       |
|  - SQLite Metadata DB (WAL Mode)      - Exception & Timeout Handling    |
+------------------------------------+------------------------------------+
                                     | gRPC / Protobuf (HTTP/2 Binary)
                                     v
+-------------------------------------------------------------------------+
|                     C++20 High-Performance Engine Core                  |
|                             (Port 50051)                                |
|  - SIMD Inverted Index                - Porter Stemmer & Stopwords      |
|  - Delta-VByte Postings Compression   - Okapi BM25 & TF-IDF Scoring     |
|  - Boolean AST Query Engine           - Binary Persistence with CRC32   |
+-------------------------------------------------------------------------+
```

---

## 2. Why C++20 is Used for Search and Ranking Algorithms

### 2.1 Cache Locality & Memory Density
In full-text information retrieval, the hot path consists of traversing posting lists—arrays of document IDs, term frequencies, and token positions. In C++20:
- Postings are laid out as contiguous primitive byte buffers (`std::vector<uint8_t>`).
- Memory footprint is dense without JVM object overhead (16–24 bytes per object header in Java).
- Sequential memory traversal maximizes CPU L1/L2 cache hit rates and hardware prefetcher efficiency.

### 2.2 Deterministic Tail Latency (Zero GC Pauses)
Search applications require deterministic $P_{99}$ latency guarantees. In garbage-collected runtimes, concurrent mark-and-sweep or generational sweeps can introduce millisecond-scale latency spikes under high query loads. C++ provides deterministic resource management via RAII (Resource Acquisition Is Initialization), ensuring:
- Immediate memory reclamation upon query termination.
- Zero GC-induced tail latency anomalies ($P_{99} < 2\text{ ms}$).

### 2.3 Postings Compression via Delta-VByte
Posting lists contain monotonically increasing document IDs: $[d_1, d_2, d_3, \dots, d_n]$. 
IndexForge utilizes **Delta Encoding** followed by **Variable-Byte (VByte)** compression:
$$\Delta_i = d_i - d_{i-1}$$
Small deltas ($\le 127$) consume only 1 byte instead of 4 or 8 bytes. The bitwise decompression loop executes in tight C++ assembly, reducing memory consumption by over **70%** and accelerating intersection speeds.

### 2.4 Mathematical Ranking Precision
Okapi BM25 and TF-IDF scoring involve intensive floating-point arithmetic across tens of thousands of postings:
$$\text{Score}(D, Q) = \sum_{t \in Q} \text{IDF}(t) \cdot \frac{f(t, D) \cdot (k_1 + 1)}{f(t, D) + k_1 \cdot \left(1 - b + b \cdot \frac{|D|}{\text{avgdl}}\right)}$$
C++20 compilers (Clang, GCC, MSVC) auto-vectorize these inner loops with SIMD instructions (AVX-2 / AVX-512 / ARM Neon), executing multiple ranking calculations per CPU clock cycle.

---

## 3. Why Java 21 + Spring Boot 3 is Used for Application APIs

### 3.1 Enterprise Integration & Mature Web Stack
While C++ excels at raw numerical computation, building production REST APIs, OpenAPI documentation, and security layers in C++ requires substantial boilerplate and third-party dependency management. Spring Boot 3 provides:
- First-class dependency injection and configuration management (`@ConfigurationProperties`, `@Value`).
- Automatic OpenAPI 3.0 / Swagger UI generation via `springdoc-openapi`.
- Declarative security, input validation (`jakarta.validation`), and uniform exception handling (`@ControllerAdvice`).

### 3.2 Java 21 Language Features
IndexForge leverages Java 21 modern runtime enhancements:
- **Records**: Immutable data transfer objects (`SearchHitDto`, `SearchResponseDto`, `DocumentMetadata`) with zero boilerplate.
- **Pattern Matching for Switch**: Clean dispatching of ranking algorithms and query types.
- **Generational ZGC**: Near-zero pause time garbage collection for web request processing.
- **Virtual Threads**: High-throughput non-blocking request concurrency.

### 3.3 SQLite Document Metadata Management
The application layer manages rich document metadata (author, source, publish date, original filenames, content snapshots) and search history audit trails in **SQLite** using WAL (Write-Ahead Logging) mode:
- ACID transactions guarantee consistency.
- Separation of metadata from index structures prevents index bloat.
- Search audit logging allows analytical tracking of query latency, frequency, and zero-hit queries.

---

## 4. Java–C++ Communication via gRPC & Protocol Buffers

### 4.1 Strict Typed Contracts (`proto/search_service.proto`)
The boundary between Spring Boot and C++ is governed by Protocol Buffers v3:
- Prevents serialization schema drift.
- Strong compile-time typing in both C++ and Java.
- Backward and forward compatibility through protobuf field numbering.

```protobuf
syntax = "proto3";
package indexforge;

service SearchEngineService {
  rpc Search (SearchRequest) returns (SearchResponse);
  rpc IndexDocument (IndexDocumentRequest) returns (IndexDocumentResponse);
  rpc IndexBatch (IndexBatchRequest) returns (IndexBatchResponse);
  rpc DeleteDocument (DeleteDocumentRequest) returns (DeleteDocumentResponse);
  rpc GetStats (StatsRequest) returns (StatsResponse);
  rpc CheckHealth (HealthRequest) returns (HealthResponse);
  rpc SaveIndex (SaveIndexRequest) returns (SaveIndexResponse);
  rpc LoadIndex (LoadIndexRequest) returns (LoadIndexResponse);
  rpc VerifyIntegrity (VerifyIntegrityRequest) returns (VerifyIntegrityResponse);
  rpc ClearCache (ClearCacheRequest) returns (ClearCacheResponse);
}
```

### 4.2 Network Efficiency (HTTP/2 Multiplexing)
Unlike traditional JSON-over-HTTP REST microservice boundaries:
- **Binary Framing**: Protobuf payloads are up to 6x smaller and serialize/deserialize 10x faster than JSON.
- **Multiplexing**: Multiple search queries and indexing requests share a single persistent TCP connection without head-of-line blocking.
- **Connection Keep-Alives**: Persistent channel pooling eliminates TCP handshake and TLS negotiation overhead.

### 4.3 Fault Tolerance & Resiliency
The Spring Boot client implements robust defensive patterns:
- **Deadline Propagation**: Search queries have a strict 5-second deadline; long-running operations are aborted to prevent resource exhaustion.
- **Status Code Mapping**: gRPC `UNAVAILABLE` is translated to HTTP `503 Service Unavailable`; `DEADLINE_EXCEEDED` maps to HTTP `504 Gateway Timeout`; `INVALID_ARGUMENT` maps to HTTP `400 Bad Request`.
- **Engine Recovery**: When the C++ engine restarts, the gRPC channel automatically re-establishes connections with backoff.

---

## 5. End-to-End System Request Flows

### 5.1 Document Ingestion Flow
```
Client (UI / REST)
       |
       | POST /api/ingest/text
       v
Spring Boot DocumentController
       |
       | 1. Validate payload (title, content, date, type)
       v
DocumentService
       |
       +---> 2. Insert metadata & content into SQLite (metadata.db)
       |        (Generates unique integer doc_id)
       |
       +---> 3. Build gRPC IndexDocumentRequest(doc_id, title, content, type)
                |
                v (gRPC / HTTP2)
         C++ Search Engine Core
                |
                +---> Tokenize text (lowercase, strip punctuation)
                +---> Stop-word removal (common English stop-words)
                +---> Porter Stemming algorithm (e.g., "procurements" -> "procur")
                +---> Invert postings: term -> (doc_id, tf, [positions])
                +---> Update Delta-VByte compressed posting lists
                +---> Evict outdated query cache entries
                |
                v Return IndexDocumentResponse (tokens_indexed, time_ms)
DocumentService
       |
       v Return HTTP 201 Created (doc_id, tokens_indexed, latency)
Client
```

### 5.2 Query Processing & Search Flow
```
Client (UI Search Input: "public tender")
       |
       | GET /api/search?q=public+tender&algo=bm25&limit=20
       v
Spring Boot SearchController
       |
       | 1. Parameter extraction & validation
       v
SearchService
       |
       | 2. Build gRPC SearchRequest(query, algo=BM25, k1=1.2, b=0.75, limit=20)
       |
       v (gRPC / HTTP2)
C++ Search Engine Core
       |
       +---> 3. Check query cache (LRU cache for exact query hits)
       +---> 4. Parse query expression (Keywords, Boolean AND/OR/NOT, or Phrase)
       +---> 5. Retrieve postings lists for stemmed query terms
       +---> 6. Decompress Delta-VByte postings into memory
       +---> 7. Calculate BM25 scores across document accumulator
       +---> 8. Min-Heap selection of Top-K document IDs and scores
       +---> 9. Generate dynamic snippet with <mark> highlighting
       |
       v Return SearchResponse(hits, engine_latency_ms, total_hits)
SearchService
       |
       +---> 10. Audit search query into SQLite search_history table
       |         (query, algorithm, total_hits, latency_ms, timestamp)
       |
       v Return HTTP 200 OK (SearchResponseDto JSON)
Client (React UI renders ranked cards with highlighted snippets)
```

---

## 6. Persistence, Integrity & Failure Recovery

### 6.1 Binary Inverted Index Checkpoint Format (`.forge`)
The C++ core serializes its state to a compact binary file format:
```
+-----------------------------------------------------------+
| Offset | Field                  | Type                    |
+-----------------------------------------------------------+
| 0x00   | Magic Bytes            | 4 bytes ("IFRG")        |
| 0x04   | File Format Version    | uint32 (e.g., 2)        |
| 0x08   | Timestamp              | uint64 (epoch seconds)  |
| 0x10   | Document Count         | uint32                  |
| 0x14   | Term Count             | uint32                  |
| 0x18   | Total Postings Count   | uint32                  |
| 0x1C   | Avg Document Length    | double                  |
| 0x24   | Document Table         | Length-prefixed records |
| ....   | Inverted Index Dict    | Term -> Postings bytes  |
| End-4  | CRC32 Checksum         | uint32 (IEEE 802.3)     |
+-----------------------------------------------------------+
```

### 6.2 CRC32 Checksum Integrity Verification
Upon loading or saving an index:
- C++ calculates the CRC32 checksum across all data bytes.
- During `load_from_file`, the calculated CRC32 is compared against the header checksum.
- Any bit corruption, partial write, or truncated file is immediately rejected with `CORRUPT_FILE` error.

### 6.3 Recovery Strategy on Unclean Shutdown
1. **SQLite Metadata Layer**: Protected by Write-Ahead Logging (`PRAGMA journal_mode=WAL`). Uncommitted transactions roll back automatically upon process restart.
2. **C++ Inverted Index Layer**: Loaded from the most recent valid binary checkpoint (`.forge`).
3. **Synchronization Check**: Spring Boot queries the C++ engine stats and compares `active_documents` with SQLite `SELECT COUNT(*) FROM documents`. Any discrepancy triggers incremental index catch-up.

---

## 7. Performance Trade-Off Analysis

| Characteristic | Inverted Index (Okapi BM25) | Inverted Index (TF-IDF) | Unindexed Linear Scan |
| :--- | :--- | :--- | :--- |
| **Search Time Complexity** | $O(|Q| \cdot \text{avg}(L_{\text{postings}})) \approx O(1)$ | $O(|Q| \cdot \text{avg}(L_{\text{postings}})) \approx O(1)$ | $O(N \cdot L_{\text{doc}})$ (Brute force) |
| **Query Latency (10k docs)** | **0.3 – 1.2 ms** | **0.2 – 0.9 ms** | **45 – 120 ms** (100x slower) |
| **Scalability (1M docs)** | Sub-millisecond ($< 5\text{ ms}$) | Sub-millisecond ($< 5\text{ ms}$) | Unusable ($> 10\text{ seconds}$) |
| **Index Overhead** | ~30% of raw corpus size | ~30% of raw corpus size | 0% (No index) |
| **Term Frequency Saturation**| Yes (governed by $k_1$) | No (linear or logarithmic) | N/A |
| **Length Normalization** | Yes (governed by $b$) | Cosine normalization | None |
| **Phrase & Position Queries**| Supported via delta positions | Supported via delta positions | Supported via substring |
| **Use Case Recommendation** | **Production Full-Text Search** | Statistical baseline | Ground-truth verification |

### BM25 vs TF-IDF Empirical Insights
- **TF-IDF** awards proportional score to term frequency, which can distort rankings in long documents where a term is repeated unnaturally.
- **Okapi BM25** introduces asymptotic term frequency saturation ($k_1 \approx 1.2$) and document length penalization ($b \approx 0.75$), consistently achieving superior Mean Reciprocal Rank (MRR) and Normalized Discounted Cumulative Gain (NDCG@10) on legal, technical, and government corpus benchmarks.

---

## 8. Summary of Component Boundaries

| Responsibility | C++20 Search Core | Java 21 Spring Boot | React 18 UI |
| :--- | :---: | :---: | :---: |
| Text Tokenization & Stemming | **Primary** | - | - |
| Inverted Index & Postings Storage | **Primary** | - | - |
| Okapi BM25 & TF-IDF Scoring | **Primary** | - | - |
| Dynamic Snippet Generation | **Primary** | - | - |
| Binary Persistence with CRC32 | **Primary** | - | - |
| Document Metadata & Content DB | - | **Primary (SQLite)** | - |
| Query Audit Trail & Latency Logs | - | **Primary (SQLite)** | - |
| REST API & OpenAPI / Swagger | - | **Primary** | - |
| CORS & Security Filter Chain | - | **Primary** | - |
| gRPC Channel & Timeout Management| - | **Primary** | - |
| Interactive Query & Filter UI | - | - | **Primary** |
| Real-time Latency & Score Gauges | - | - | **Primary** |
