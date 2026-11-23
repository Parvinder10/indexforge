import React, { useState } from 'react';
import { runLiveBenchmark } from '../api';

interface BenchmarkDashboardProps {
  isOpen: boolean;
  onClose: () => void;
}

export const BenchmarkDashboard: React.FC<BenchmarkDashboardProps> = ({
  isOpen,
  onClose,
}) => {
  const [activeTab, setActiveTab] = useState<'1k' | '10k'>('1k');
  const [running, setRunning] = useState(false);
  const [liveOutput, setLiveOutput] = useState<string | null>(null);

  if (!isOpen) return null;

  const handleRunLive = async () => {
    setRunning(true);
    setLiveOutput(null);
    try {
      const data = await runLiveBenchmark(activeTab === '1k' ? 1000 : 10000);
      setLiveOutput(data.stdout || data.stderr || 'Benchmark completed.');
    } catch (e) {
      setLiveOutput(`Error running live benchmark: ${e}`);
    } finally {
      setRunning(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm animate-fadeIn">
      <div className="relative w-full max-w-4xl bg-slate-900 border border-slate-700/80 rounded-2xl shadow-2xl overflow-hidden flex flex-col max-h-[90vh]">
        {/* Header */}
        <div className="p-5 border-b border-slate-800 flex items-center justify-between">
          <div className="flex items-center gap-2">
            <div className="w-8 h-8 rounded-lg bg-purple-600/20 text-purple-400 flex items-center justify-center font-bold">
              ⚡
            </div>
            <div>
              <h2 className="text-lg font-bold text-slate-100">Performance Evaluation & Scaling Benchmark</h2>
              <p className="text-xs text-slate-400">Actual measured metrics on real hardware across varying corpus sizes</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-1 rounded-lg text-slate-400 hover:text-slate-100 hover:bg-slate-800 transition-colors"
          >
            ✕
          </button>
        </div>

        {/* System & Hardware Specs Banner */}
        <div className="px-6 py-3 bg-slate-950/60 border-b border-slate-800 flex flex-wrap items-center justify-between text-xs text-slate-400">
          <div className="flex items-center gap-3">
            <span><strong>Compiler:</strong> Clang 23.1 (LLVM-MinGW UCRT)</span>
            <span>•</span>
            <span><strong>Standard:</strong> C++20 (-O3 -march=native)</span>
            <span>•</span>
            <span><strong>Architecture:</strong> x86_64 Multi-threaded</span>
          </div>
          <div className="flex items-center gap-2 mt-1 sm:mt-0">
            <div className="inline-flex rounded-lg bg-slate-900 p-0.5 border border-slate-800 text-xs font-medium">
              <button
                onClick={() => setActiveTab('1k')}
                className={`px-3 py-1 rounded-md transition-all ${
                  activeTab === '1k' ? 'bg-blue-600 text-white' : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                1,000 Docs
              </button>
              <button
                onClick={() => setActiveTab('10k')}
                className={`px-3 py-1 rounded-md transition-all ${
                  activeTab === '10k' ? 'bg-blue-600 text-white' : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                10,000 Docs
              </button>
            </div>
          </div>
        </div>

        {/* Benchmark Body */}
        <div className="p-6 overflow-y-auto space-y-6">
          {activeTab === '1k' ? (
            /* 1,000 Documents Data */
            <div className="space-y-6">
              {/* Highlight Cards */}
              <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
                <div className="p-4 bg-slate-950/60 border border-slate-800/80 rounded-xl">
                  <div className="text-xs text-slate-400 font-medium">Speedup vs Linear Scan</div>
                  <div className="text-3xl font-extrabold text-emerald-400 mt-1">80.6x</div>
                  <p className="text-[11px] text-slate-500 mt-1">1.15 ms BM25 vs 92.77 ms Linear scan</p>
                </div>
                <div className="p-4 bg-slate-950/60 border border-slate-800/80 rounded-xl">
                  <div className="text-xs text-slate-400 font-medium">Indexing Throughput</div>
                  <div className="text-3xl font-extrabold text-blue-400 mt-1">7,730</div>
                  <p className="text-[11px] text-slate-500 mt-1">docs/sec (129.4 ms total build time)</p>
                </div>
                <div className="p-4 bg-slate-950/60 border border-slate-800/80 rounded-xl">
                  <div className="text-xs text-slate-400 font-medium">Concurrent Throughput</div>
                  <div className="text-3xl font-extrabold text-purple-400 mt-1">2,665</div>
                  <p className="text-[11px] text-slate-500 mt-1">QPS across 4 parallel reader threads</p>
                </div>
              </div>

              {/* Comparative Table */}
              <div className="space-y-2">
                <h3 className="text-xs font-semibold text-slate-300">Detailed Latency & Retrieval Distribution (1,000 docs)</h3>
                <div className="overflow-x-auto rounded-xl border border-slate-800 bg-slate-950/40 text-xs">
                  <table className="w-full text-left">
                    <thead className="bg-slate-900/80 text-slate-400 font-semibold border-b border-slate-800">
                      <tr>
                        <th className="p-3">Retrieval Strategy</th>
                        <th className="p-3">Mean Latency</th>
                        <th className="p-3">p50 Latency</th>
                        <th className="p-3">p95 Latency</th>
                        <th className="p-3">p99 Latency</th>
                        <th className="p-3">Throughput (QPS)</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-slate-800/60 font-mono">
                      <tr className="hover:bg-slate-900/30">
                        <td className="p-3 font-sans font-medium text-amber-300">Linear Scan Baseline</td>
                        <td className="p-3">92.77 ms</td>
                        <td className="p-3">87.34 ms</td>
                        <td className="p-3">125.46 ms</td>
                        <td className="p-3">125.46 ms</td>
                        <td className="p-3">10.8 QPS</td>
                      </tr>
                      <tr className="hover:bg-slate-900/30 bg-blue-500/5">
                        <td className="p-3 font-sans font-medium text-blue-400">Inverted Index (BM25)</td>
                        <td className="p-3 text-emerald-400 font-semibold">1.15 ms</td>
                        <td className="p-3 text-emerald-400 font-semibold">1.11 ms</td>
                        <td className="p-3">1.97 ms</td>
                        <td className="p-3">2.31 ms</td>
                        <td className="p-3 text-emerald-400 font-semibold">868.9 QPS</td>
                      </tr>
                      <tr className="hover:bg-slate-900/30">
                        <td className="p-3 font-sans font-medium text-purple-400">Inverted Index (TF-IDF)</td>
                        <td className="p-3">1.22 ms</td>
                        <td className="p-3">1.14 ms</td>
                        <td className="p-3">2.30 ms</td>
                        <td className="p-3">2.58 ms</td>
                        <td className="p-3">819.5 QPS</td>
                      </tr>
                    </tbody>
                  </table>
                </div>
              </div>

              {/* Memory and Index Size Breakdown */}
              <div className="grid grid-cols-2 gap-4 text-xs">
                <div className="p-3 bg-slate-950/40 border border-slate-800 rounded-xl">
                  <span className="text-slate-400">In-Memory Index Size:</span>
                  <div className="text-lg font-bold text-slate-200 mt-0.5">2.53 MB</div>
                  <p className="text-[11px] text-slate-500">Includes term dictionary, postings, and doc store</p>
                </div>
                <div className="p-3 bg-slate-950/40 border border-slate-800 rounded-xl">
                  <span className="text-slate-400">VByte Compressed Disk File:</span>
                  <div className="text-lg font-bold text-slate-200 mt-0.5">1.13 MB</div>
                  <p className="text-[11px] text-slate-500">55.3% compression ratio vs raw structs</p>
                </div>
              </div>
            </div>
          ) : (
            /* 10,000 Documents Data */
            <div className="space-y-6">
              {/* Highlight Cards */}
              <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
                <div className="p-4 bg-slate-950/60 border border-slate-800/80 rounded-xl">
                  <div className="text-xs text-slate-400 font-medium">Speedup vs Linear Scan</div>
                  <div className="text-3xl font-extrabold text-emerald-400 mt-1">58.2x</div>
                  <p className="text-[11px] text-slate-500 mt-1">17.29 ms BM25 vs 1,005.70 ms Linear scan</p>
                </div>
                <div className="p-4 bg-slate-950/60 border border-slate-800/80 rounded-xl">
                  <div className="text-xs text-slate-400 font-medium">Indexing Throughput</div>
                  <div className="text-3xl font-extrabold text-blue-400 mt-1">8,402</div>
                  <p className="text-[11px] text-slate-500 mt-1">docs/sec (1,190.2 ms for 10k documents)</p>
                </div>
                <div className="p-4 bg-slate-950/60 border border-slate-800/80 rounded-xl">
                  <div className="text-xs text-slate-400 font-medium">Postings Managed</div>
                  <div className="text-3xl font-extrabold text-purple-400 mt-1">287,489</div>
                  <p className="text-[11px] text-slate-500 mt-1">Across 953,280 token occurrences</p>
                </div>
              </div>

              {/* Comparative Table */}
              <div className="space-y-2">
                <h3 className="text-xs font-semibold text-slate-300">Detailed Latency & Retrieval Distribution (10,000 docs)</h3>
                <div className="overflow-x-auto rounded-xl border border-slate-800 bg-slate-950/40 text-xs">
                  <table className="w-full text-left">
                    <thead className="bg-slate-900/80 text-slate-400 font-semibold border-b border-slate-800">
                      <tr>
                        <th className="p-3">Retrieval Strategy</th>
                        <th className="p-3">Mean Latency</th>
                        <th className="p-3">p50 Latency</th>
                        <th className="p-3">p95 Latency</th>
                        <th className="p-3">p99 Latency</th>
                        <th className="p-3">Throughput (QPS)</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-slate-800/60 font-mono">
                      <tr className="hover:bg-slate-900/30">
                        <td className="p-3 font-sans font-medium text-amber-300">Linear Scan Baseline</td>
                        <td className="p-3 text-rose-400">1,005.70 ms</td>
                        <td className="p-3">1,029.07 ms</td>
                        <td className="p-3">1,132.15 ms</td>
                        <td className="p-3">1,132.15 ms</td>
                        <td className="p-3">0.99 QPS</td>
                      </tr>
                      <tr className="hover:bg-slate-900/30 bg-blue-500/5">
                        <td className="p-3 font-sans font-medium text-blue-400">Inverted Index (BM25)</td>
                        <td className="p-3 text-emerald-400 font-semibold">17.29 ms</td>
                        <td className="p-3 text-emerald-400 font-semibold">16.91 ms</td>
                        <td className="p-3">29.35 ms</td>
                        <td className="p-3">34.11 ms</td>
                        <td className="p-3 text-emerald-400 font-semibold">57.8 QPS</td>
                      </tr>
                      <tr className="hover:bg-slate-900/30">
                        <td className="p-3 font-sans font-medium text-purple-400">Inverted Index (TF-IDF)</td>
                        <td className="p-3">16.88 ms</td>
                        <td className="p-3">16.33 ms</td>
                        <td className="p-3">30.40 ms</td>
                        <td className="p-3">35.48 ms</td>
                        <td className="p-3">59.2 QPS</td>
                      </tr>
                    </tbody>
                  </table>
                </div>
              </div>

              {/* Memory and Index Size Breakdown */}
              <div className="grid grid-cols-2 gap-4 text-xs">
                <div className="p-3 bg-slate-950/40 border border-slate-800 rounded-xl">
                  <span className="text-slate-400">In-Memory Index Size:</span>
                  <div className="text-lg font-bold text-slate-200 mt-0.5">31.0 MB</div>
                  <p className="text-[11px] text-slate-500">~3.1 KB per full document indexed</p>
                </div>
                <div className="p-3 bg-slate-950/40 border border-slate-800 rounded-xl">
                  <span className="text-slate-400">VByte Compressed Disk File:</span>
                  <div className="text-lg font-bold text-slate-200 mt-0.5">11.2 MB</div>
                  <p className="text-[11px] text-slate-500">63.8% space reduction with delta-VByte</p>
                </div>
              </div>
            </div>
          )}

          {/* Live Run Benchmark Section */}
          <div className="pt-4 border-t border-slate-800 space-y-3">
            <div className="flex items-center justify-between">
              <div>
                <h4 className="text-xs font-semibold text-slate-200">Execute Live Benchmark Run</h4>
                <p className="text-[11px] text-slate-400">Runs real-time comparative benchmark on the C++ engine core</p>
              </div>
              <button
                onClick={handleRunLive}
                disabled={running}
                className="px-4 py-2 bg-purple-600 hover:bg-purple-500 disabled:bg-slate-800 text-white text-xs font-medium rounded-xl shadow transition-all flex items-center gap-1.5"
              >
                <span>{running ? 'Benchmarking in C++...' : 'Run Live Benchmark'}</span>
              </button>
            </div>

            {liveOutput && (
              <pre className="p-4 bg-slate-950 border border-slate-800 rounded-xl text-[11px] font-mono text-emerald-300 overflow-x-auto max-h-48 whitespace-pre-wrap">
                {liveOutput}
              </pre>
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
