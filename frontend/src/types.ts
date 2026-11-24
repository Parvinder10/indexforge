export interface SearchHit {
  doc_id: number;
  docId?: number;
  score: number;
  title: string;
  source: string;
  date: string;
  doc_type: string;
  docType?: string;
  snippet: string;
  doc_length: number;
  length?: number;
  matched_terms: string[];
  matchedTerms?: string[];
}

export interface SearchResponse {
  query: string;
  algorithm: string;
  k1?: number;
  b?: number;
  total_hits: number;
  totalHits?: number;
  engine_latency_ms: number;
  engineLatencyMs?: number;
  total_latency_ms: number;
  totalLatencyMs?: number;
  from_cache: boolean;
  fromCache?: boolean;
  hits: SearchHit[];
}

export interface IndexStats {
  total_documents: number;
  active_documents: number;
  deleted_documents: number;
  total_unique_terms: number;
  total_postings: number;
  total_token_occurrences: number;
  avg_doc_length: number;
  estimated_memory_bytes: number;
  cache_entries: number;
  cache_hits: number;
  cache_misses: number;
  sqlite_documents?: number;
  available_doc_types?: string[];
}

export interface DocumentItem {
  id: number;
  title: string;
  source: string;
  date: string;
  doc_type: string;
  file_name?: string;
  content: string;
  length: number;
  indexed_at?: string;
}

export interface BenchmarkMetrics {
  mean_ms: number;
  p50_ms: number;
  p95_ms: number;
  p99_ms: number;
  qps: number;
}

export interface BenchmarkResult {
  corpus_size: number;
  indexing_time_ms: number;
  docs_per_sec: number;
  memory_mb: number;
  index_size_mb: number;
  linear: BenchmarkMetrics;
  bm25: BenchmarkMetrics;
  tfidf: BenchmarkMetrics;
  speedup: number;
  concurrent_qps: number;
}

export interface SearchHistoryItem {
  id: number;
  query: string;
  algorithm: string;
  totalHits: number;
  latencyMs: number;
  searchedAt: string;
}
