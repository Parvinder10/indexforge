import React from 'react';
import { SearchHit } from '../types';

interface SearchResultsProps {
  hits: SearchHit[];
  totalHits: number;
  loading: boolean;
  query: string;
  onSelectDoc: (docId: number) => void;
}

const TYPE_COLORS: Record<string, { bg: string; text: string; border: string }> = {
  tender: { bg: 'bg-emerald-500/10', text: 'text-emerald-300', border: 'border-emerald-500/30' },
  notice: { bg: 'bg-blue-500/10', text: 'text-blue-300', border: 'border-blue-500/30' },
  policy: { bg: 'bg-purple-500/10', text: 'text-purple-300', border: 'border-purple-500/30' },
  regulation: { bg: 'bg-amber-500/10', text: 'text-amber-300', border: 'border-amber-500/30' },
  tech_doc: { bg: 'bg-cyan-500/10', text: 'text-cyan-300', border: 'border-cyan-500/30' },
};

export const SearchResults: React.FC<SearchResultsProps> = ({
  hits,
  totalHits,
  loading,
  query,
  onSelectDoc,
}) => {
  if (loading) {
    return (
      <div className="w-full max-w-5xl mx-auto py-12 space-y-4">
        {[1, 2, 3].map((i) => (
          <div key={i} className="p-6 bg-slate-900/60 border border-slate-800 rounded-2xl animate-pulse space-y-3">
            <div className="h-5 bg-slate-800 rounded w-2/5" />
            <div className="h-4 bg-slate-800/60 rounded w-full" />
            <div className="h-4 bg-slate-800/40 rounded w-4/5" />
            <div className="flex gap-2 pt-2">
              <div className="h-6 bg-slate-800 rounded-full w-20" />
              <div className="h-6 bg-slate-800 rounded-full w-28" />
            </div>
          </div>
        ))}
      </div>
    );
  }

  if (hits.length === 0) {
    return (
      <div className="w-full max-w-5xl mx-auto py-16 text-center space-y-4 bg-slate-900/40 border border-slate-800/80 rounded-2xl p-8">
        <div className="w-16 h-16 mx-auto rounded-full bg-slate-800/80 flex items-center justify-center text-slate-500">
          <svg className="w-8 h-8" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M9.172 16.172a4 4 0 015.656 0M9 10h.01M15 10h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
          </svg>
        </div>
        <h3 className="text-lg font-semibold text-slate-200">No matching documents found</h3>
        <p className="text-sm text-slate-400 max-w-md mx-auto">
          {query ? (
            <>
              No documents matched query <span className="text-blue-400 font-mono">"{query}"</span>. Try adjusting keywords, using <span className="font-semibold text-slate-300">OR</span> expressions, or clearing document type filters.
            </>
          ) : (
            'Enter a search query above to query the inverted index.'
          )}
        </p>
      </div>
    );
  }

  // Calculate highest score to normalize score bars
  const maxScore = Math.max(...hits.map((h) => h.score), 1.0);

  return (
    <div className="w-full max-w-5xl mx-auto space-y-4">
      <div className="flex items-center justify-between text-xs text-slate-400 px-1">
        <span>Showing <strong className="text-slate-200">{hits.length}</strong> of <strong className="text-slate-200">{totalHits}</strong> matching documents</span>
      </div>
      {hits.map((hit, idx) => {
        const typeStyle = TYPE_COLORS[hit.doc_type] || {
          bg: 'bg-slate-500/10',
          text: 'text-slate-300',
          border: 'border-slate-500/30',
        };
        const scorePct = Math.min(100, Math.round((hit.score / maxScore) * 100));

        return (
          <article
            key={hit.doc_id}
            onClick={() => onSelectDoc(hit.doc_id)}
            className="group relative p-5 bg-slate-900/80 hover:bg-slate-900 border border-slate-800 hover:border-slate-700/80 rounded-2xl transition-all shadow-md hover:shadow-xl cursor-pointer"
          >
            {/* Header: Title and Relevance Score */}
            <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-3">
              <div className="space-y-1">
                <div className="flex items-center gap-2">
                  <span className="text-xs font-mono text-slate-500">#{idx + 1}</span>
                  <h2 className="text-base sm:text-lg font-semibold text-slate-100 group-hover:text-blue-400 transition-colors">
                    {hit.title}
                  </h2>
                </div>
                {/* Metadata Row */}
                <div className="flex flex-wrap items-center gap-2 text-xs text-slate-400">
                  <span className={`px-2 py-0.5 rounded-md border font-medium uppercase tracking-wider text-[10px] ${typeStyle.bg} ${typeStyle.text} ${typeStyle.border}`}>
                    {(hit.doc_type || hit.docType || 'policy').replace('_', ' ')}
                  </span>
                  <span>•</span>
                  <span>{hit.source}</span>
                  <span>•</span>
                  <span>{hit.date}</span>
                  {((hit.doc_length || hit.length || 0) > 0) && (
                    <>
                      <span>•</span>
                      <span>{hit.doc_length || hit.length} tokens</span>
                    </>
                  )}
                </div>
              </div>

              {/* Score Indicator */}
              <div className="flex sm:flex-col items-center sm:items-end justify-between gap-1 text-right min-w-[90px]">
                <div className="text-xs font-mono text-slate-400">
                  Relevance: <span className="font-semibold text-blue-400">{hit.score.toFixed(3)}</span>
                </div>
                <div className="w-20 bg-slate-800 h-1.5 rounded-full overflow-hidden">
                  <div
                    className="bg-gradient-to-r from-blue-500 to-indigo-500 h-full rounded-full transition-all"
                    style={{ width: `${scorePct}%` }}
                  />
                </div>
              </div>
            </div>

            {/* Snippet with <mark> tags */}
            {hit.snippet && (
              <div
                className="mt-3 text-sm text-slate-300 leading-relaxed font-sans search-snippet"
                dangerouslySetInnerHTML={{ __html: hit.snippet }}
              />
            )}

            {/* Footer: Matched Terms and Inspect Button */}
            <div className="mt-4 pt-3 border-t border-slate-800/80 flex flex-wrap items-center justify-between gap-2 text-xs">
              <div className="flex flex-wrap items-center gap-1.5">
                <span className="text-slate-500 text-[11px]">Matched terms:</span>
                {(hit.matched_terms || hit.matchedTerms || []).map((term, tIdx) => (
                  <span
                    key={tIdx}
                    className="px-2 py-0.5 rounded bg-slate-800/80 text-blue-300 border border-slate-700/50 font-mono text-[11px]"
                  >
                    {term}
                  </span>
                ))}
              </div>

              <span className="text-blue-400 group-hover:translate-x-0.5 transition-transform flex items-center gap-1 font-medium text-[11px]">
                View Document
                <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M9 5l7 7-7 7" />
                </svg>
              </span>
            </div>
          </article>
        );
      })}
    </div>
  );
};
