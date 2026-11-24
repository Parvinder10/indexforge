import React, { useEffect, useState } from 'react';
import { SearchHistoryItem } from '../types';
import { getSearchHistory, clearSearchHistory } from '../api';

interface SearchHistoryModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSelectQuery: (q: string) => void;
}

export const SearchHistoryModal: React.FC<SearchHistoryModalProps> = ({
  isOpen,
  onClose,
  onSelectQuery,
}) => {
  const [history, setHistory] = useState<SearchHistoryItem[]>([]);
  const [loading, setLoading] = useState(false);

  const loadHistory = async () => {
    setLoading(true);
    try {
      const data = await getSearchHistory(50);
      setHistory(data);
    } catch (e) {
      console.error('Failed to load search history:', e);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (isOpen) {
      loadHistory();
    }
  }, [isOpen]);

  if (!isOpen) return null;

  const handleClear = async () => {
    if (window.confirm('Are you sure you want to clear search history?')) {
      await clearSearchHistory();
      setHistory([]);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm animate-fadeIn">
      <div className="relative w-full max-w-2xl bg-slate-900 border border-slate-700/80 rounded-2xl shadow-2xl overflow-hidden flex flex-col max-h-[85vh]">
        {/* Header */}
        <div className="p-5 border-b border-slate-800 flex items-center justify-between">
          <div className="flex items-center gap-2">
            <div className="w-8 h-8 rounded-lg bg-cyan-600/20 text-cyan-400 flex items-center justify-center font-bold">
              📜
            </div>
            <div>
              <h2 className="text-lg font-bold text-slate-100">Search Query History</h2>
              <p className="text-xs text-slate-400">Recorded by Java Spring Boot & SQLite Audit Store</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-1 rounded-lg text-slate-400 hover:text-slate-100 hover:bg-slate-800 transition-colors"
          >
            ✕
          </button>
        </div>

        {/* Content */}
        <div className="p-6 overflow-y-auto space-y-3 flex-1">
          {loading ? (
            <div className="text-center py-8 text-slate-400 text-sm">Loading search history...</div>
          ) : history.length === 0 ? (
            <div className="text-center py-12 text-slate-500 text-sm">No recent search history recorded yet.</div>
          ) : (
            <div className="divide-y divide-slate-800/80">
              {history.map((item) => (
                <div
                  key={item.id}
                  onClick={() => {
                    onSelectQuery(item.query);
                    onClose();
                  }}
                  className="py-3 px-3 -mx-3 rounded-xl hover:bg-slate-800/50 cursor-pointer flex items-center justify-between transition-colors group"
                >
                  <div className="space-y-0.5">
                    <div className="font-medium text-slate-200 group-hover:text-blue-400 text-sm">
                      "{item.query}"
                    </div>
                    <div className="text-[11px] text-slate-400 flex items-center gap-2">
                      <span className="px-1.5 py-0.2 rounded bg-slate-800 uppercase font-mono text-[9px] text-slate-300">
                        {item.algorithm}
                      </span>
                      <span>•</span>
                      <span>{item.totalHits} hits</span>
                      <span>•</span>
                      <span>{item.latencyMs.toFixed(2)} ms</span>
                    </div>
                  </div>
                  <div className="text-[11px] text-slate-500 font-mono">
                    {item.searchedAt.replace('T', ' ').substring(0, 19)}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>

        {/* Footer */}
        <div className="p-4 border-t border-slate-800 bg-slate-950/50 flex items-center justify-between">
          <button
            onClick={handleClear}
            disabled={history.length === 0}
            className="px-3.5 py-1.5 text-xs text-red-400 hover:text-red-300 hover:bg-red-950/30 rounded-lg transition-colors disabled:opacity-40"
          >
            Clear History
          </button>
          <button
            onClick={onClose}
            className="px-4 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium rounded-xl transition-colors"
          >
            Close
          </button>
        </div>
      </div>
    </div>
  );
};
