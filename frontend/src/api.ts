import { SearchResponse, IndexStats, DocumentItem } from './types';

const API_BASE = (import.meta as any).env?.VITE_API_BASE_URL || '/api';

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
  return res.json();
}

export async function searchLinear(q: string, limit: number = 20): Promise<SearchResponse> {
  const query = new URLSearchParams({ q, limit: limit.toString() });
  const res = await fetch(`${API_BASE}/search/linear?${query.toString()}`);
  if (!res.ok) throw new Error(`Linear search failed: ${res.statusText}`);
  return res.json();
}

export async function getStats(): Promise<IndexStats> {
  const res = await fetch(`${API_BASE}/stats`);
  if (!res.ok) throw new Error('Failed to load stats');
  return res.json();
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
