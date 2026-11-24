import React, { useState, useEffect, useCallback } from 'react';
import { SearchBar } from './components/SearchBar';
import { SearchResults } from './components/SearchResults';
import { DocumentViewerModal } from './components/DocumentViewerModal';
import { IndexStatsModal } from './components/IndexStatsModal';
import { IngestionModal } from './components/IngestionModal';
import { BenchmarkDashboard } from './components/BenchmarkDashboard';
import { SearchHistoryModal } from './components/SearchHistoryModal';
import { search, searchLinear, getStats } from './api';
import { SearchHit, IndexStats } from './types';

export const App: React.FC = () => {
  const [query, setQuery] = useState('public tender');
  const [algorithm, setAlgorithm] = useState<'bm25' | 'tfidf'>('bm25');
  const [k1, setK1] = useState(1.2);
  const [b, setB] = useState(0.75);
  const [selectedType, setSelectedType] = useState('all');
  const [isLinear, setIsLinear] = useState(false);

  const [hits, setHits] = useState<SearchHit[]>([]);
  const [totalHits, setTotalHits] = useState(0);
  const [latencyMs, setLatencyMs] = useState<number | undefined>(undefined);
  const [fromCache, setFromCache] = useState(false);
  const [loading, setLoading] = useState(false);

  const [selectedDocId, setSelectedDocId] = useState<number | null>(null);
  const [stats, setStats] = useState<IndexStats | null>(null);
  const [showStatsModal, setShowStatsModal] = useState(false);
  const [showIngestionModal, setShowIngestionModal] = useState(false);
  const [showBenchmarkModal, setShowBenchmarkModal] = useState(false);
  const [showHistoryModal, setShowHistoryModal] = useState(false);

  const fetchStats = useCallback(async () => {
    try {
      const data = await getStats();
      setStats(data);
    } catch (e) {
      console.error('Failed to fetch stats:', e);
    }
  }, []);

  const handleSearch = useCallback(
    async (overrideQuery?: string) => {
      const q = overrideQuery !== undefined ? overrideQuery : query;
      if (!q.trim()) {
        setHits([]);
        setTotalHits(0);
        setLatencyMs(undefined);
        return;
      }

      setLoading(true);
      try {
        if (isLinear) {
          const res = await searchLinear(q, 20);
          setHits(res.hits);
          setTotalHits(res.total_hits);
          setLatencyMs(res.engine_latency_ms);
          setFromCache(false);
        } else {
          const res = await search({
            q,
            algo: algorithm,
            k1,
            b,
            doc_type: selectedType,
            limit: 20,
          });
          setHits(res.hits);
          setTotalHits(res.total_hits);
          setLatencyMs(res.engine_latency_ms);
          setFromCache(res.from_cache);
        }
      } catch (err) {
        console.error('Search error:', err);
      } finally {
        setLoading(false);
      }
    },
    [query, algorithm, k1, b, selectedType, isLinear]
  );

  useEffect(() => {
    fetchStats();
    handleSearch();
  }, []);

  useEffect(() => {
    handleSearch();
  }, [algorithm, k1, b, selectedType, isLinear]);

  const handleDocDeleted = () => {
    fetchStats();
    handleSearch();
  };

  const handleDocIngested = () => {
    fetchStats();
    handleSearch();
  };

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 flex flex-col font-sans">
      {/* Top Navigation Bar */}
      <header className="sticky top-0 z-40 bg-slate-950/80 backdrop-blur-md border-b border-slate-800/80 px-4 sm:px-8 py-3.5">
        <div className="max-w-7xl mx-auto flex items-center justify-between gap-4">
          {/* Logo & Brand */}
          <div className="flex items-center gap-3">
            <div className="w-10 h-10 rounded-xl bg-gradient-to-tr from-blue-600 via-indigo-600 to-cyan-400 flex items-center justify-center shadow-lg shadow-blue-500/20">
              <svg className="w-6 h-6 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <polygon strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" points="12 2 2 7 12 12 22 7 12 2" />
                <polyline strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" points="2 17 12 22 22 17" />
                <polyline strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" points="2 12 12 17 22 12" />
              </svg>
            </div>
            <div>
              <div className="flex items-center gap-2">
                <h1 className="font-extrabold text-lg tracking-tight bg-gradient-to-r from-white via-slate-200 to-slate-400 bg-clip-text text-transparent">
                  IndexForge
                </h1>
                <span className="px-2 py-0.5 rounded text-[10px] font-semibold bg-blue-500/10 text-blue-400 border border-blue-500/30">
                  C++20 + Java 21
                </span>
              </div>
              <p className="text-[11px] text-slate-400 hidden sm:block">
                Hybrid Architecture • C++ Inverted Index • Spring Boot Orchestration • gRPC
              </p>
            </div>
          </div>

          {/* Action Buttons */}
          <div className="flex items-center gap-2 sm:gap-3 text-xs font-medium">
            <button
              onClick={() => setShowBenchmarkModal(true)}
              className="px-3.5 py-2 rounded-xl bg-purple-600/10 hover:bg-purple-600/20 text-purple-300 border border-purple-500/30 transition-all flex items-center gap-1.5 shadow-sm"
            >
              <span>⚡ Benchmarks</span>
            </button>

            <button
              onClick={() => setShowHistoryModal(true)}
              className="px-3.5 py-2 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-200 border border-slate-800 transition-all flex items-center gap-1.5 shadow-sm"
              title="Search History Audit Log"
            >
              <span>📜 History</span>
            </button>

            <button
              onClick={() => setShowStatsModal(true)}
              className="px-3.5 py-2 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-200 border border-slate-800 transition-all flex items-center gap-1.5 shadow-sm"
            >
              <span>📊 Stats</span>
              {stats?.active_documents !== undefined && (
                <span className="px-1.5 py-0.2 rounded bg-slate-800 text-[10px] font-mono text-blue-400">
                  {stats.active_documents}
                </span>
              )}
            </button>

            <button
              onClick={() => setShowIngestionModal(true)}
              className="px-4 py-2 rounded-xl bg-blue-600 hover:bg-blue-500 text-white transition-all shadow-md shadow-blue-600/20 flex items-center gap-1.5"
            >
              <span>+ Ingest</span>
            </button>
          </div>
        </div>
      </header>

      {/* Main Container */}
      <main className="flex-1 max-w-7xl w-full mx-auto px-4 sm:px-8 py-8 space-y-6">
        {/* Search Bar Section */}
        <section>
          <SearchBar
            query={query}
            onQueryChange={setQuery}
            onSearch={handleSearch}
            algorithm={algorithm}
            onAlgorithmChange={(a) => setAlgorithm(a as 'bm25' | 'tfidf')}
            k1={k1}
            onK1Change={setK1}
            b={b}
            onBChange={setB}
            selectedType={selectedType}
            onTypeChange={setSelectedType}
            isLinear={isLinear}
            onToggleLinear={setIsLinear}
            latencyMs={latencyMs}
            totalHits={totalHits}
            fromCache={fromCache}
          />
        </section>

        {/* Results Section */}
        <section>
          <SearchResults
            hits={hits}
            totalHits={totalHits}
            loading={loading}
            query={query}
            onSelectDoc={(id) => setSelectedDocId(id)}
          />
        </section>
      </main>

      {/* Footer */}
      <footer className="mt-auto border-t border-slate-800/80 bg-slate-950/60 py-6 text-center text-xs text-slate-500">
        <div className="max-w-7xl mx-auto px-4 flex flex-col sm:flex-row items-center justify-between gap-3">
          <div>
            IndexForge Search Platform • C++20 Core • Okapi BM25 & TF-IDF • Delta-VByte Compression
          </div>
          <div className="flex items-center gap-4 text-slate-400 font-mono text-[11px]">
            <span>Java 21 Spring Boot</span>
            <span>•</span>
            <span>gRPC / Protobuf</span>
            <span>•</span>
            <span>SQLite Metadata</span>
            <span>•</span>
            <span>React + TypeScript</span>
          </div>
        </div>
      </footer>

      {/* Modals */}
      <DocumentViewerModal
        docId={selectedDocId}
        onClose={() => setSelectedDocId(null)}
        onDeleted={handleDocDeleted}
      />

      <IndexStatsModal
        isOpen={showStatsModal}
        onClose={() => setShowStatsModal(false)}
        stats={stats}
        onRefresh={fetchStats}
      />

      <SearchHistoryModal
        isOpen={showHistoryModal}
        onClose={() => setShowHistoryModal(false)}
        onSelectQuery={(q) => {
          setQuery(q);
          handleSearch(q);
        }}
      />

      <IngestionModal
        isOpen={showIngestionModal}
        onClose={() => setShowIngestionModal(false)}
        onIngested={handleDocIngested}
      />

      <BenchmarkDashboard
        isOpen={showBenchmarkModal}
        onClose={() => setShowBenchmarkModal(false)}
      />
    </div>
  );
};

export default App;
