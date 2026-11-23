#pragma once

#include "indexforge/types.hpp"
#include "indexforge/document.hpp"
#include "indexforge/posting.hpp"
#include "indexforge/tokenizer.hpp"
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>
#include <optional>

namespace indexforge {

class InvertedIndex {
public:
    explicit InvertedIndex(Tokenizer tokenizer = Tokenizer{});

    // Ingest a document incrementally
    bool add_document(Document doc);

    // Delete a document by DocId (soft delete / tombstoning for O(1) time)
    bool delete_document(DocId id);

    // Update document content/metadata
    bool update_document(Document doc);

    // Query postings list for a processed term (thread-safe read)
    // Returns a copy of postings for the term excluding tombstoned documents
    PostingList get_postings(const std::string& term) const;

    // Fast check for term existence and document frequency
    size_t get_doc_frequency(const std::string& term) const;

    // Check if document exists and is not deleted
    bool has_document(DocId id) const;
    bool is_deleted(DocId id) const;

    // Retrieve document metadata and raw content
    std::optional<DocumentMeta> get_document_meta(DocId id) const;
    std::optional<Document> get_document(DocId id) const;

    // Global corpus statistics
    size_t total_documents() const;
    size_t active_documents() const;
    size_t deleted_documents() const;
    size_t total_unique_terms() const;
    size_t total_token_count() const;
    double avg_doc_length() const;
    IndexStats get_stats() const;

    // Memory compaction: remove deleted documents from postings lists
    void compact();

    // Clear all index data
    void clear();

    const Tokenizer& tokenizer() const { return tokenizer_; }

    // Direct read-only access with locked reader context for batch queries
    std::shared_mutex& mutex() const { return mutex_; }
    const std::unordered_map<std::string, PostingList>& raw_index() const { return index_; }
    const std::unordered_map<DocId, Document>& doc_store() const { return documents_; }
    const std::unordered_set<DocId>& deleted_set() const { return deleted_docs_; }

    // Load internal state directly (used by deserializer)
    void restore_state(
        std::unordered_map<std::string, PostingList> index,
        std::unordered_map<DocId, Document> documents,
        std::unordered_set<DocId> deleted_docs,
        size_t total_tokens);

private:
    Tokenizer tokenizer_;
    mutable std::shared_mutex mutex_;

    // Core inverted index: term -> postings list
    std::unordered_map<std::string, PostingList> index_;

    // Document store
    std::unordered_map<DocId, Document> documents_;

    // Tombstone set for deleted documents
    std::unordered_set<DocId> deleted_docs_;

    // Running token counter
    size_t total_tokens_{0};
};

} // namespace indexforge
