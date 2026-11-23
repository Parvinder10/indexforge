import React, { useEffect, useState } from 'react';
import { DocumentItem } from '../types';
import { getDocument, deleteDocument } from '../api';

interface DocumentViewerModalProps {
  docId: number | null;
  onClose: () => void;
  onDeleted: (docId: number) => void;
}

export const DocumentViewerModal: React.FC<DocumentViewerModalProps> = ({
  docId,
  onClose,
  onDeleted,
}) => {
  const [doc, setDoc] = useState<DocumentItem | null>(null);
  const [loading, setLoading] = useState(false);
  const [deleting, setDeleting] = useState(false);

  useEffect(() => {
    if (docId !== null) {
      setLoading(true);
      getDocument(docId)
        .then((data) => setDoc(data))
        .catch((err) => console.error(err))
        .finally(() => setLoading(false));
    } else {
      setDoc(null);
    }
  }, [docId]);

  if (docId === null) return null;

  const handleDelete = async () => {
    if (!window.confirm(`Are you sure you want to delete document #${docId}? This will remove it from the inverted index and SQLite storage.`)) {
      return;
    }
    setDeleting(true);
    try {
      await deleteDocument(docId);
      onDeleted(docId);
      onClose();
    } catch (err) {
      alert(`Failed to delete document: ${err}`);
    } finally {
      setDeleting(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm animate-fadeIn">
      <div className="relative w-full max-w-4xl max-h-[85vh] bg-slate-900 border border-slate-700/80 rounded-2xl shadow-2xl flex flex-col overflow-hidden">
        {/* Header */}
        <div className="p-5 border-b border-slate-800 flex items-start justify-between gap-4">
          <div>
            <div className="flex items-center gap-2 mb-1">
              <span className="px-2 py-0.5 rounded text-[10px] font-mono uppercase bg-blue-500/20 text-blue-300 border border-blue-500/30">
                Doc #{docId}
              </span>
              {doc?.doc_type && (
                <span className="px-2 py-0.5 rounded text-[10px] uppercase font-semibold bg-slate-800 text-slate-300">
                  {doc.doc_type}
                </span>
              )}
            </div>
            <h2 className="text-xl font-bold text-slate-100">
              {loading ? 'Loading document...' : doc?.title || 'Document Details'}
            </h2>
          </div>
          <button
            onClick={onClose}
            className="p-1.5 rounded-lg text-slate-400 hover:text-slate-100 hover:bg-slate-800 transition-colors"
          >
            <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M6 18L18 6M6 6l12 12" />
            </svg>
          </button>
        </div>

        {/* Metadata Summary */}
        {doc && (
          <div className="px-5 py-3 bg-slate-950/50 border-b border-slate-800 flex flex-wrap items-center gap-4 text-xs text-slate-400">
            <div><strong className="text-slate-300">Source:</strong> {doc.source || 'N/A'}</div>
            <div><strong className="text-slate-300">Date:</strong> {doc.date || 'N/A'}</div>
            <div><strong className="text-slate-300">Length:</strong> {doc.length} tokens</div>
            {doc.file_name && <div><strong className="text-slate-300">File:</strong> {doc.file_name}</div>}
          </div>
        )}

        {/* Content Body */}
        <div className="p-6 overflow-y-auto flex-1 font-mono text-xs leading-relaxed text-slate-200 whitespace-pre-wrap selection:bg-blue-600 selection:text-white">
          {loading ? (
            <div className="space-y-3 animate-pulse">
              <div className="h-4 bg-slate-800 rounded w-full" />
              <div className="h-4 bg-slate-800 rounded w-5/6" />
              <div className="h-4 bg-slate-800 rounded w-4/6" />
            </div>
          ) : (
            doc?.content || 'No content available for this document.'
          )}
        </div>

        {/* Footer Actions */}
        <div className="p-4 border-t border-slate-800 bg-slate-900/90 flex items-center justify-between">
          <button
            type="button"
            onClick={handleDelete}
            disabled={deleting}
            className="px-3.5 py-2 rounded-xl text-xs font-medium bg-rose-600/10 hover:bg-rose-600/20 text-rose-300 border border-rose-500/30 transition-all flex items-center gap-1.5"
          >
            <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16" />
            </svg>
            <span>{deleting ? 'Deleting...' : 'Delete Document'}</span>
          </button>

          <button
            type="button"
            onClick={onClose}
            className="px-5 py-2 rounded-xl text-xs font-medium bg-slate-800 hover:bg-slate-700 text-slate-200 transition-colors"
          >
            Close
          </button>
        </div>
      </div>
    </div>
  );
};
