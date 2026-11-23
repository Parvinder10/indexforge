import React, { useState } from 'react';
import { IndexStats } from '../types';
import { clearCache, verifyIndexIntegrity, saveIndex, loadIndex } from '../api';

interface IndexStatsModalProps {
  isOpen: boolean;
  onClose: () => void;
  stats: IndexStats | null;
  onRefresh: () => void;
}

export const IndexStatsModal: React.FC<IndexStatsModalProps> = ({
  isOpen,
  onClose,
  stats,
  onRefresh,
}) => {
  const [integrityMsg, setIntegrityMsg] = useState<string | null>(null);
  const [actionLoading, setActionLoading] = useState(false);

  if (!isOpen) return null;

  const handleVerify = async () => {
    setActionLoading(true);
    setIntegrityMsg(null);
    try {
      const res = await verifyIndexIntegrity();
      if (res.valid) {
        setIntegrityMsg(`✅ Integrity Verified: Checksum ${res.checksum} matches, ${res.bytes_checked} bytes valid.`);
      } else {
        setIntegrityMsg(`❌ Integrity Check Failed: ${res.error || 'Corrupted file'}`);
      }
    } catch (e) {
      setIntegrityMsg(`❌ Error: ${e}`);
    } finally {
      setActionLoading(false);
    }
  };

  const handleClearCache = async () => {
    setActionLoading(true);
    await clearCache();
    onRefresh();
    setActionLoading(false);
  };

  const handleSave = async () => {
    setActionLoading(true);
    const res = await saveIndex();
    alert(res.status === 'saved' ? 'Index successfully saved to index.forge' : 'Failed to save index');
    setActionLoading(false);
  };

  const handleLoad = async () => {
    setActionLoading(true);
    const res = await loadIndex();
    alert(res.status === 'loaded' ? 'Index successfully reloaded from index.forge' : 'Failed to load index');
    onRefresh();
    setActionLoading(false);
  };

  const totalLookups = (stats?.cache_hits || 0) + (stats?.cache_misses || 0);
  const hitRate = totalLookups > 0 ? (((stats?.cache_hits || 0) / totalLookups) * 100).toFixed(1) : '0.0';
  const memoryMb = stats ? (stats.estimated_memory_bytes / 1024 / 1024).toFixed(2) : '0.00';

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm animate-fadeIn">
      <div className="relative w-full max-w-3xl bg-slate-900 border border-slate-700/80 rounded-2xl shadow-2xl overflow-hidden flex flex-col">
        {/* Header */}
        <div className="p-5 border-b border-slate-800 flex items-center justify-between">
          <div className="flex items-center gap-2">
            <div className="w-8 h-8 rounded-lg bg-blue-600/20 text-blue-400 flex items-center justify-center font-bold">
              📊
            </div>
            <h2 className="text-lg font-bold text-slate-100">Inverted Index Corpus & Engine Health</h2>
          </div>
          <button
            onClick={onClose}
            className="p-1 rounded-lg text-slate-400 hover:text-slate-100 hover:bg-slate-800 transition-colors"
          >
            ✕
          </button>
        </div>

        {/* Content */}
        <div className="p-6 space-y-6 overflow-y-auto max-h-[70vh]">
          {/* Key Metric Grid */}
          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 text-center">
            <div className="p-4 bg-slate-950/60 rounded-xl border border-slate-800/80">
              <div className="text-2xl font-bold text-blue-400">{stats?.active_documents || 0}</div>
              <div className="text-xs text-slate-400 mt-1">Active Documents</div>
            </div>
            <div className="p-4 bg-slate-950/60 rounded-xl border border-slate-800/80">
              <div className="text-2xl font-bold text-emerald-400">{stats?.total_unique_terms || 0}</div>
              <div className="text-xs text-slate-400 mt-1">Unique Terms</div>
            </div>
            <div className="p-4 bg-slate-950/60 rounded-xl border border-slate-800/80">
              <div className="text-2xl font-bold text-purple-400">{stats?.total_postings || 0}</div>
              <div className="text-xs text-slate-400 mt-1">Total Postings</div>
            </div>
            <div className="p-4 bg-slate-950/60 rounded-xl border border-slate-800/80">
              <div className="text-2xl font-bold text-amber-400">{memoryMb} MB</div>
              <div className="text-xs text-slate-400 mt-1">Memory Footprint</div>
            </div>
          </div>

          {/* Details Table */}
          <div className="space-y-2 text-xs">
            <h3 className="font-semibold text-slate-300">Engine Statistics Breakdown</h3>
            <div className="divide-y divide-slate-800 bg-slate-950/40 rounded-xl border border-slate-800 overflow-hidden">
              <div className="flex justify-between px-4 py-2.5">
                <span className="text-slate-400">Total Indexed Documents (lifetime):</span>
                <span className="text-slate-200 font-mono">{stats?.total_documents || 0}</span>
              </div>
              <div className="flex justify-between px-4 py-2.5">
                <span className="text-slate-400">Tombstoned / Deleted Documents:</span>
                <span className="text-slate-200 font-mono">{stats?.deleted_documents || 0}</span>
              </div>
              <div className="flex justify-between px-4 py-2.5">
                <span className="text-slate-400">Total Indexed Token Occurrences:</span>
                <span className="text-slate-200 font-mono">{stats?.total_token_occurrences || 0}</span>
              </div>
              <div className="flex justify-between px-4 py-2.5">
                <span className="text-slate-400">Average Document Length:</span>
                <span className="text-slate-200 font-mono">{stats?.avg_doc_length.toFixed(1) || 0} tokens</span>
              </div>
              <div className="flex justify-between px-4 py-2.5">
                <span className="text-slate-400">Metadata Storage:</span>
                <span className="text-slate-200 font-mono">SQLite (WAL Mode, {stats?.sqlite_documents || 0} records)</span>
              </div>
            </div>
          </div>

          {/* Cache Metrics */}
          <div className="space-y-2 text-xs">
            <div className="flex justify-between items-center">
              <h3 className="font-semibold text-slate-300">LRU Query Result Cache</h3>
              <button
                onClick={handleClearCache}
                disabled={actionLoading}
                className="text-blue-400 hover:underline"
              >
                Clear Cache
              </button>
            </div>
            <div className="grid grid-cols-4 gap-2 text-center">
              <div className="p-3 bg-slate-950/40 border border-slate-800 rounded-lg">
                <div className="font-bold text-slate-200">{stats?.cache_entries || 0}</div>
                <div className="text-[10px] text-slate-500">Cached Entries</div>
              </div>
              <div className="p-3 bg-slate-950/40 border border-slate-800 rounded-lg">
                <div className="font-bold text-emerald-400">{stats?.cache_hits || 0}</div>
                <div className="text-[10px] text-slate-500">Cache Hits</div>
              </div>
              <div className="p-3 bg-slate-950/40 border border-slate-800 rounded-lg">
                <div className="font-bold text-slate-400">{stats?.cache_misses || 0}</div>
                <div className="text-[10px] text-slate-500">Cache Misses</div>
              </div>
              <div className="p-3 bg-slate-950/40 border border-slate-800 rounded-lg">
                <div className="font-bold text-blue-400">{hitRate}%</div>
                <div className="text-[10px] text-slate-500">Hit Rate</div>
              </div>
            </div>
          </div>

          {/* Integrity and Persistence Tools */}
          <div className="space-y-3 pt-2 border-t border-slate-800 text-xs">
            <h3 className="font-semibold text-slate-300">Index Persistence & Integrity Operations</h3>
            <div className="flex flex-wrap gap-2">
              <button
                type="button"
                onClick={handleVerify}
                disabled={actionLoading}
                className="px-3.5 py-2 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-xl transition-all font-medium flex items-center gap-1.5"
              >
                <span>🔍 Verify CRC32 Integrity</span>
              </button>
              <button
                type="button"
                onClick={handleSave}
                disabled={actionLoading}
                className="px-3.5 py-2 bg-blue-600/20 hover:bg-blue-600/30 text-blue-300 border border-blue-500/30 rounded-xl transition-all font-medium"
              >
                💾 Save Index (index.forge)
              </button>
              <button
                type="button"
                onClick={handleLoad}
                disabled={actionLoading}
                className="px-3.5 py-2 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-xl transition-all font-medium"
              >
                📂 Reload Index
              </button>
            </div>
            {integrityMsg && (
              <div className="p-3 bg-slate-950 border border-slate-800 rounded-lg font-mono text-[11px] text-slate-300">
                {integrityMsg}
              </div>
            )}
          </div>
        </div>

        {/* Footer */}
        <div className="p-4 border-t border-slate-800 bg-slate-950/50 flex justify-end">
          <button
            onClick={onClose}
            className="px-5 py-2 bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium rounded-xl transition-colors"
          >
            Close
          </button>
        </div>
      </div>
    </div>
  );
};
