import React, { useState } from 'react';

interface SearchBarProps {
  query: string;
  onQueryChange: (q: string) => void;
  onSearch: (overrideQuery?: string) => void;
  algorithm: string;
  onAlgorithmChange: (algo: string) => void;
  k1: number;
  onK1Change: (k1: number) => void;
  b: number;
  onBChange: (b: number) => void;
  selectedType: string;
  onTypeChange: (type: string) => void;
  isLinear: boolean;
  onToggleLinear: (linear: boolean) => void;
  latencyMs?: number;
  totalHits?: number;
  fromCache?: boolean;
}

const DOC_TYPES = [
  { id: 'all', label: 'All Documents' },
  { id: 'tender', label: 'Public Tenders' },
  { id: 'notice', label: 'Gov Notices' },
  { id: 'policy', label: 'Policies' },
  { id: 'regulation', label: 'Regulations' },
  { id: 'tech_doc', label: 'Technical Docs' },
];

export const SearchBar: React.FC<SearchBarProps> = ({
  query,
  onQueryChange,
  onSearch,
  algorithm,
  onAlgorithmChange,
  k1,
  onK1Change,
  b,
  onBChange,
  selectedType,
  onTypeChange,
  isLinear,
  onToggleLinear,
  latencyMs,
  totalHits,
  fromCache,
}) => {
  const [showSyntax, setShowSyntax] = useState(false);
  const [showTuning, setShowTuning] = useState(false);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSearch();
  };

  const handleExampleClick = (ex: string) => {
    onQueryChange(ex);
    onSearch(ex);
    setShowSyntax(false);
  };

  return (
    <div className="w-full max-w-5xl mx-auto space-y-4">
      {/* Search Input Box */}
      <form onSubmit={handleSubmit} className="relative flex items-center">
        <div className="relative flex-1">
          <input
            type="text"
            value={query}
            onChange={(e) => onQueryChange(e.target.value)}
            placeholder='Search documents (e.g. "public tender", federal AND cybersecurity, grant OR policy)...'
            className="w-full pl-12 pr-28 py-3.5 bg-slate-900 border border-slate-700/80 rounded-xl text-slate-100 placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent text-base shadow-lg transition-all"
          />
          <div className="absolute left-4 top-1/2 -translate-y-1/2 text-slate-400">
            <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
            </svg>
          </div>
          {query && (
            <button
              type="button"
              onClick={() => onQueryChange('')}
              className="absolute right-20 top-1/2 -translate-y-1/2 text-slate-400 hover:text-slate-200 p-1"
            >
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M6 18L18 6M6 6l12 12" />
              </svg>
            </button>
          )}
        </div>
        <button
          type="submit"
          className="ml-3 px-6 py-3.5 bg-blue-600 hover:bg-blue-500 active:bg-blue-700 text-white font-medium rounded-xl shadow-md transition-all flex items-center gap-2"
        >
          <span>Search</span>
          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M14 5l7 7m0 0l-7 7m7-7H3" />
          </svg>
        </button>
      </form>

      {/* Query Controls & Stats Bar */}
      <div className="flex flex-wrap items-center justify-between gap-3 text-sm">
        {/* Left: Mode, Algorithm, & Tuning Buttons */}
        <div className="flex flex-wrap items-center gap-2">
          {/* Baseline Toggle */}
          <button
            type="button"
            onClick={() => onToggleLinear(!isLinear)}
            className={`px-3 py-1.5 rounded-lg border text-xs font-medium transition-all flex items-center gap-1.5 ${
              isLinear
                ? 'bg-amber-500/20 border-amber-500/50 text-amber-300'
                : 'bg-slate-900 border-slate-700 text-slate-300 hover:border-slate-600'
            }`}
          >
            <span className={`w-2 h-2 rounded-full ${isLinear ? 'bg-amber-400 animate-pulse' : 'bg-slate-500'}`} />
            <span>{isLinear ? 'Mode: Linear Scan Baseline' : 'Mode: Inverted Index'}</span>
          </button>

          {!isLinear && (
            <>
              {/* Ranking Algorithm Switcher */}
              <div className="inline-flex rounded-lg bg-slate-900 p-0.5 border border-slate-800 text-xs">
                <button
                  type="button"
                  onClick={() => onAlgorithmChange('bm25')}
                  className={`px-3 py-1 rounded-md font-medium transition-all ${
                    algorithm === 'bm25'
                      ? 'bg-blue-600 text-white shadow-sm'
                      : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  Okapi BM25
                </button>
                <button
                  type="button"
                  onClick={() => onAlgorithmChange('tfidf')}
                  className={`px-3 py-1 rounded-md font-medium transition-all ${
                    algorithm === 'tfidf'
                      ? 'bg-blue-600 text-white shadow-sm'
                      : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  TF-IDF
                </button>
              </div>

              {/* Tuning Parameters Popover Toggle */}
              {algorithm === 'bm25' && (
                <button
                  type="button"
                  onClick={() => setShowTuning(!showTuning)}
                  className={`px-2.5 py-1.5 rounded-lg border text-xs flex items-center gap-1.5 transition-all ${
                    showTuning
                      ? 'bg-blue-500/10 border-blue-500/40 text-blue-300'
                      : 'bg-slate-900 border-slate-800 text-slate-400 hover:text-slate-300'
                  }`}
                >
                  <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
                  </svg>
                  <span>k₁={k1.toFixed(2)} b={b.toFixed(2)}</span>
                </button>
              )}
            </>
          )}

          {/* Syntax Guide Button */}
          <button
            type="button"
            onClick={() => setShowSyntax(!showSyntax)}
            className="px-2.5 py-1.5 rounded-lg bg-slate-900 border border-slate-800 text-xs text-slate-400 hover:text-slate-300 transition-all flex items-center gap-1"
          >
            <svg className="w-3.5 h-3.5 text-blue-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
            <span>Syntax Help</span>
          </button>
        </div>

        {/* Right: Latency & Hit Badges */}
        {latencyMs !== undefined && (
          <div className="flex items-center gap-2 text-xs">
            {fromCache && (
              <span className="px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-300 border border-emerald-500/30 flex items-center gap-1">
                <span>⚡ Cached</span>
              </span>
            )}
            <span className="px-2.5 py-1 rounded-lg bg-slate-900 border border-slate-800 text-slate-300 font-mono">
              <span className="text-blue-400 font-semibold">{latencyMs.toFixed(2)} ms</span> latency
            </span>
            {totalHits !== undefined && (
              <span className="px-2.5 py-1 rounded-lg bg-slate-900 border border-slate-800 text-slate-300">
                <span className="text-slate-100 font-semibold">{totalHits}</span> results
              </span>
            )}
          </div>
        )}
      </div>

      {/* BM25 Tuning Sliders Drawer */}
      {showTuning && algorithm === 'bm25' && !isLinear && (
        <div className="p-4 bg-slate-900/90 border border-slate-800 rounded-xl space-y-3 text-xs shadow-inner">
          <div className="flex items-center justify-between text-slate-300 font-medium">
            <span>BM25 Ranking Hyperparameters</span>
            <button
              onClick={() => { onK1Change(1.2); onBChange(0.75); }}
              className="text-blue-400 hover:underline text-xs"
            >
              Reset to Defaults (k₁=1.2, b=0.75)
            </button>
          </div>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <div className="flex justify-between text-slate-400 mb-1">
                <span>Term Frequency Saturation (k₁): <strong className="text-slate-200">{k1.toFixed(2)}</strong></span>
                <span className="text-slate-500">Default: 1.2</span>
              </div>
              <input
                type="range"
                min="0.1"
                max="3.0"
                step="0.05"
                value={k1}
                onChange={(e) => onK1Change(parseFloat(e.target.value))}
                className="w-full accent-blue-500 bg-slate-800 rounded-lg"
              />
              <p className="text-slate-500 text-[11px] mt-0.5">Controls how quickly term frequency saturates. Higher values reward repeated terms more.</p>
            </div>
            <div>
              <div className="flex justify-between text-slate-400 mb-1">
                <span>Document Length Normalization (b): <strong className="text-slate-200">{b.toFixed(2)}</strong></span>
                <span className="text-slate-500">Default: 0.75</span>
              </div>
              <input
                type="range"
                min="0.0"
                max="1.0"
                step="0.05"
                value={b}
                onChange={(e) => onBChange(parseFloat(e.target.value))}
                className="w-full accent-blue-500 bg-slate-800 rounded-lg"
              />
              <p className="text-slate-500 text-[11px] mt-0.5">0.0 = no length penalty; 1.0 = full penalty for long documents compared to average length.</p>
            </div>
          </div>
        </div>
      )}

      {/* Syntax Guide Popover */}
      {showSyntax && (
        <div className="p-4 bg-slate-900 border border-slate-800 rounded-xl space-y-3 text-xs shadow-xl">
          <div className="flex justify-between items-center text-slate-300 font-semibold border-b border-slate-800 pb-2">
            <span>Query Syntax Reference</span>
            <button onClick={() => setShowSyntax(false)} className="text-slate-400 hover:text-slate-200">✕</button>
          </div>
          <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-3">
            <div className="p-2.5 bg-slate-950/60 rounded-lg border border-slate-800/80">
              <span className="text-blue-400 font-semibold block mb-1">Exact Phrase</span>
              <code className="text-amber-300 bg-slate-900 px-1.5 py-0.5 rounded cursor-pointer" onClick={() => handleExampleClick('"public tender"')}>
                "public tender"
              </code>
              <p className="text-slate-400 mt-1">Requires exact consecutive token adjacency verified via position index.</p>
            </div>
            <div className="p-2.5 bg-slate-950/60 rounded-lg border border-slate-800/80">
              <span className="text-emerald-400 font-semibold block mb-1">Boolean Conjunction</span>
              <code className="text-amber-300 bg-slate-900 px-1.5 py-0.5 rounded cursor-pointer" onClick={() => handleExampleClick('federal AND procurement')}>
                federal AND procurement
              </code>
              <p className="text-slate-400 mt-1">Matches documents containing both terms via postings intersection.</p>
            </div>
            <div className="p-2.5 bg-slate-950/60 rounded-lg border border-slate-800/80">
              <span className="text-purple-400 font-semibold block mb-1">Boolean Disjunction</span>
              <code className="text-amber-300 bg-slate-900 px-1.5 py-0.5 rounded cursor-pointer" onClick={() => handleExampleClick('notice OR tender')}>
                notice OR tender
              </code>
              <p className="text-slate-400 mt-1">Matches documents containing either term via postings union.</p>
            </div>
            <div className="p-2.5 bg-slate-950/60 rounded-lg border border-slate-800/80">
              <span className="text-rose-400 font-semibold block mb-1">Negation</span>
              <code className="text-amber-300 bg-slate-900 px-1.5 py-0.5 rounded cursor-pointer" onClick={() => handleExampleClick('cloud AND NOT legacy')}>
                cloud AND NOT legacy
              </code>
              <p className="text-slate-400 mt-1">Excludes documents containing the negated term.</p>
            </div>
            <div className="p-2.5 bg-slate-950/60 rounded-lg border border-slate-800/80">
              <span className="text-cyan-400 font-semibold block mb-1">Grouping</span>
              <code className="text-amber-300 bg-slate-900 px-1.5 py-0.5 rounded cursor-pointer" onClick={() => handleExampleClick('(tender OR grant) AND federal')}>
                (tender OR grant) AND federal
              </code>
              <p className="text-slate-400 mt-1">Nested parenthesized boolean logic expression.</p>
            </div>
            <div className="p-2.5 bg-slate-950/60 rounded-lg border border-slate-800/80">
              <span className="text-amber-400 font-semibold block mb-1">Keywords</span>
              <code className="text-amber-300 bg-slate-900 px-1.5 py-0.5 rounded cursor-pointer" onClick={() => handleExampleClick('cybersecurity compliance')}>
                cybersecurity compliance
              </code>
              <p className="text-slate-400 mt-1">Multi-keyword free query ranked by relevance scoring.</p>
            </div>
          </div>
        </div>
      )}

      {/* Document Type Filter Chips */}
      <div className="flex flex-wrap items-center gap-2 pt-1">
        <span className="text-xs text-slate-400 mr-1">Filter Type:</span>
        {DOC_TYPES.map((t) => (
          <button
            key={t.id}
            type="button"
            onClick={() => onTypeChange(t.id)}
            className={`px-3 py-1 rounded-full text-xs font-medium transition-all ${
              selectedType === t.id
                ? 'bg-blue-600/30 border border-blue-500 text-blue-200'
                : 'bg-slate-900/60 border border-slate-800 text-slate-400 hover:text-slate-200 hover:border-slate-700'
            }`}
          >
            {t.label}
          </button>
        ))}
      </div>
    </div>
  );
};
