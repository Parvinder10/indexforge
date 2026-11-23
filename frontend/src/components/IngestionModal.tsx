import React, { useState } from 'react';
import { ingestText, ingestSampleDocuments } from '../api';

interface IngestionModalProps {
  isOpen: boolean;
  onClose: () => void;
  onIngested: () => void;
}

export const IngestionModal: React.FC<IngestionModalProps> = ({
  isOpen,
  onClose,
  onIngested,
}) => {
  const [title, setTitle] = useState('');
  const [source, setSource] = useState('Government Repository');
  const [date, setDate] = useState('2026-06-01');
  const [docType, setDocType] = useState('tender');
  const [content, setContent] = useState('');
  const [loading, setLoading] = useState(false);
  const [statusMsg, setStatusMsg] = useState<string | null>(null);

  if (!isOpen) return null;

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!content.trim() || !title.trim()) {
      alert('Title and content are required.');
      return;
    }
    setLoading(true);
    setStatusMsg(null);
    try {
      const res = await ingestText({
        title,
        source,
        date,
        doc_type: docType,
        content,
      });
      if (res.success) {
        setStatusMsg(`✅ Successfully ingested Document #${res.doc_id}: "${res.title}"`);
        setTitle('');
        setContent('');
        onIngested();
      } else {
        setStatusMsg('❌ Failed to ingest document.');
      }
    } catch (err) {
      setStatusMsg(`❌ Error: ${err}`);
    } finally {
      setLoading(false);
    }
  };

  const handleImportSamples = async () => {
    setLoading(true);
    setStatusMsg(null);
    try {
      const res = await ingestSampleDocuments();
      setStatusMsg(`✅ Successfully imported ${res.ingested_count} standard public documents!`);
      onIngested();
    } catch (err) {
      setStatusMsg(`❌ Error importing samples: ${err}`);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm animate-fadeIn">
      <div className="relative w-full max-w-2xl bg-slate-900 border border-slate-700/80 rounded-2xl shadow-2xl overflow-hidden flex flex-col">
        {/* Header */}
        <div className="p-5 border-b border-slate-800 flex items-center justify-between">
          <div className="flex items-center gap-2">
            <div className="w-8 h-8 rounded-lg bg-emerald-600/20 text-emerald-400 flex items-center justify-center font-bold">
              📥
            </div>
            <h2 className="text-lg font-bold text-slate-100">Document Ingestion Pipeline</h2>
          </div>
          <button
            onClick={onClose}
            className="p-1 rounded-lg text-slate-400 hover:text-slate-100 hover:bg-slate-800 transition-colors"
          >
            ✕
          </button>
        </div>

        {/* Body */}
        <div className="p-6 space-y-5 overflow-y-auto max-h-[75vh]">
          {/* One-click Sample Dataset Banner */}
          <div className="p-4 bg-gradient-to-r from-blue-900/30 to-indigo-900/30 border border-blue-800/40 rounded-xl flex items-center justify-between gap-4">
            <div>
              <h4 className="text-xs font-semibold text-blue-200">Load Built-in Real-World Dataset</h4>
              <p className="text-[11px] text-slate-400 mt-0.5">
                Quickly import public tenders, regulations, notices, and NIST/DoD policies into the index.
              </p>
            </div>
            <button
              type="button"
              onClick={handleImportSamples}
              disabled={loading}
              className="px-3.5 py-1.5 bg-blue-600 hover:bg-blue-500 text-white text-xs font-medium rounded-lg whitespace-nowrap shadow transition-all"
            >
              Import 8 Docs
            </button>
          </div>

          {statusMsg && (
            <div className="p-3 bg-slate-950 border border-slate-800 rounded-lg text-xs font-mono text-slate-200">
              {statusMsg}
            </div>
          )}

          {/* Form */}
          <form onSubmit={handleSubmit} className="space-y-4 text-xs">
            <div>
              <label className="block text-slate-300 font-medium mb-1">Document Title *</label>
              <input
                type="text"
                required
                value={title}
                onChange={(e) => setTitle(e.target.value)}
                placeholder="e.g. Department of Energy Clean Grid Solicitation"
                className="w-full px-3 py-2 bg-slate-950 border border-slate-800 rounded-lg text-slate-100 focus:outline-none focus:ring-2 focus:ring-blue-500"
              />
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-3 gap-3">
              <div>
                <label className="block text-slate-300 font-medium mb-1">Source Agency</label>
                <input
                  type="text"
                  value={source}
                  onChange={(e) => setSource(e.target.value)}
                  placeholder="e.g. OMB, NIST, GSA"
                  className="w-full px-3 py-2 bg-slate-950 border border-slate-800 rounded-lg text-slate-100 focus:outline-none focus:ring-2 focus:ring-blue-500"
                />
              </div>

              <div>
                <label className="block text-slate-300 font-medium mb-1">Effective Date</label>
                <input
                  type="date"
                  value={date}
                  onChange={(e) => setDate(e.target.value)}
                  className="w-full px-3 py-2 bg-slate-950 border border-slate-800 rounded-lg text-slate-100 focus:outline-none focus:ring-2 focus:ring-blue-500"
                />
              </div>

              <div>
                <label className="block text-slate-300 font-medium mb-1">Document Type</label>
                <select
                  value={docType}
                  onChange={(e) => setDocType(e.target.value)}
                  className="w-full px-3 py-2 bg-slate-950 border border-slate-800 rounded-lg text-slate-100 focus:outline-none focus:ring-2 focus:ring-blue-500"
                >
                  <option value="tender">Public Tender</option>
                  <option value="notice">Government Notice</option>
                  <option value="policy">Policy Document</option>
                  <option value="regulation">Public Regulation</option>
                  <option value="tech_doc">Technical Specification</option>
                </select>
              </div>
            </div>

            <div>
              <label className="block text-slate-300 font-medium mb-1">Document Content *</label>
              <textarea
                required
                rows={7}
                value={content}
                onChange={(e) => setContent(e.target.value)}
                placeholder="Paste raw unstructured text, policy guidelines, or technical requirements..."
                className="w-full px-3 py-2 bg-slate-950 border border-slate-800 rounded-lg text-slate-100 font-mono text-xs focus:outline-none focus:ring-2 focus:ring-blue-500"
              />
            </div>

            <div className="flex justify-end gap-2 pt-2">
              <button
                type="button"
                onClick={onClose}
                className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-slate-300 font-medium rounded-xl transition-colors"
              >
                Cancel
              </button>
              <button
                type="submit"
                disabled={loading}
                className="px-5 py-2 bg-blue-600 hover:bg-blue-500 text-white font-medium rounded-xl shadow transition-all flex items-center gap-1.5"
              >
                <span>{loading ? 'Processing...' : 'Ingest & Index'}</span>
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};
