import { SearchResponse, SearchHit, IndexStats, DocumentItem, SearchHistoryItem } from './types';

const API_BASE = (import.meta as any).env?.VITE_API_BASE_URL || '/api';

function normalizeSearchHit(hit: any): SearchHit {
  const docType = hit.doc_type || hit.docType || 'policy';
  const docId = hit.doc_id ?? hit.docId ?? 0;
  const score = typeof hit.score === 'number' ? hit.score : parseFloat(hit.score) || 0;
  const length = hit.doc_length ?? hit.length ?? 0;
  const matchedTerms = Array.isArray(hit.matched_terms)
    ? hit.matched_terms
    : Array.isArray(hit.matchedTerms)
    ? hit.matchedTerms
    : [];

  return {
    doc_id: docId,
    docId,
    score,
    title: hit.title || `Document #${docId}`,
    source: hit.source || 'Federal Repository',
    date: hit.date || '2026-06-01',
    doc_type: docType,
    docType,
    snippet: hit.snippet || '',
    doc_length: length,
    length,
    matched_terms: matchedTerms,
    matchedTerms,
  };
}

function normalizeSearchResponse(data: any): SearchResponse {
  const hits = Array.isArray(data.hits) ? data.hits.map(normalizeSearchHit) : [];
  const totalHits = data.total_hits ?? data.totalHits ?? hits.length;
  const engineLatencyMs = data.engine_latency_ms ?? data.engineLatencyMs ?? 0;
  const totalLatencyMs = data.total_latency_ms ?? data.totalLatencyMs ?? engineLatencyMs;
  const fromCache = data.from_cache ?? data.fromCache ?? false;

  return {
    query: data.query || '',
    algorithm: data.algorithm || 'bm25',
    k1: data.k1,
    b: data.b,
    total_hits: totalHits,
    totalHits,
    engine_latency_ms: engineLatencyMs,
    engineLatencyMs,
    total_latency_ms: totalLatencyMs,
    totalLatencyMs,
    from_cache: fromCache,
    fromCache,
    hits,
  };
}

export async function search(params: {
  q: string;
  algo?: string;
  k1?: number;
  b?: number;
  doc_type?: string;
  limit?: number;
  offset?: number;
}): Promise<SearchResponse> {
  const query = new URLSearchParams();
  query.set('q', params.q);
  if (params.algo) query.set('algo', params.algo);
  if (params.k1 !== undefined) query.set('k1', params.k1.toString());
  if (params.b !== undefined) query.set('b', params.b.toString());
  if (params.doc_type && params.doc_type !== 'all') query.set('doc_type', params.doc_type);
  if (params.limit) query.set('limit', params.limit.toString());
  if (params.offset) query.set('offset', params.offset.toString());

  const res = await fetch(`${API_BASE}/search?${query.toString()}`);
  if (!res.ok) throw new Error(`Search failed: ${res.statusText}`);
  const data = await res.json();
  return normalizeSearchResponse(data);
}

export async function searchLinear(q: string, limit: number = 20): Promise<SearchResponse> {
  const query = new URLSearchParams({ q, limit: limit.toString() });
  const res = await fetch(`${API_BASE}/search/linear?${query.toString()}`);
  if (!res.ok) throw new Error(`Linear search failed: ${res.statusText}`);
  const data = await res.json();
  return normalizeSearchResponse(data);
}

export async function getStats(): Promise<IndexStats> {
  // Support both Spring Boot /api/index/stats and fallback /api/stats
  try {
    const res = await fetch(`${API_BASE}/index/stats`);
    if (res.ok) return await res.json();
  } catch (_) {}

  const fallback = await fetch(`${API_BASE}/stats`);
  if (!fallback.ok) throw new Error('Failed to load stats');
  return fallback.json();
}

export async function getDocument(id: number): Promise<DocumentItem> {
  const res = await fetch(`${API_BASE}/documents/${id}`);
  if (!res.ok) throw new Error(`Failed to load document #${id}`);
  return res.json();
}

export async function deleteDocument(id: number): Promise<boolean> {
  const res = await fetch(`${API_BASE}/documents/${id}`, { method: 'DELETE' });
  return res.ok;
}

export async function ingestText(doc: {
  title: string;
  source: string;
  date: string;
  doc_type: string;
  content: string;
}) {
  const res = await fetch(`${API_BASE}/ingest/text`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(doc),
  });
  if (!res.ok) throw new Error('Ingestion failed');
  return res.json();
}

export async function ingestSampleDocuments() {
  const res = await fetch(`${API_BASE}/ingest/sample`, { method: 'POST' });
  if (!res.ok) throw new Error('Failed to import sample documents');
  return res.json();
}

export async function clearCache() {
  const res = await fetch(`${API_BASE}/cache/clear`, { method: 'POST' });
  return res.json();
}

export async function verifyIndexIntegrity() {
  const res = await fetch(`${API_BASE}/index/verify`, { method: 'POST' });
  return res.json();
}

export async function saveIndex() {
  const res = await fetch(`${API_BASE}/index/save`, { method: 'POST' });
  return res.json();
}

export async function loadIndex() {
  const res = await fetch(`${API_BASE}/index/load`, { method: 'POST' });
  return res.json();
}

export async function runLiveBenchmark(corpusSize: number) {
  const res = await fetch(`${API_BASE}/benchmark`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ corpus_size: corpusSize }),
  });
  if (!res.ok) throw new Error(`Benchmark failed: ${res.statusText}`);
  return res.json();
}

export async function getSearchHistory(limit: number = 20): Promise<SearchHistoryItem[]> {
  const res = await fetch(`${API_BASE}/search/history?limit=${limit}`);
  if (!res.ok) throw new Error('Failed to load search history');
  const data = await res.json();
  return Array.isArray(data) ? data : [];
}

export async function clearSearchHistory() {
  const res = await fetch(`${API_BASE}/search/history`, { method: 'DELETE' });
  return res.json();
}
