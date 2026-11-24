#include "indexforge/inverted_index.hpp"
#include <map>
#include <numeric>

namespace indexforge {

InvertedIndex::InvertedIndex(Tokenizer tokenizer)
    : tokenizer_(std::move(tokenizer)) {}

bool InvertedIndex::add_document(Document doc) {
    if (doc.id == INVALID_DOC_ID) return false;

    std::unique_lock<std::shared_mutex> lock(mutex_);

    // If document already exists and was active, adjust token count
    if (auto it = documents_.find(doc.id); it != documents_.end()) {
        if (deleted_docs_.find(doc.id) == deleted_docs_.end()) {
            if (total_tokens_ >= it->second.length) {
                total_tokens_ -= it->second.length;
            }
        } else {
            deleted_docs_.erase(doc.id);
        }
    }

    // Tokenize document content
    std::vector<Token> tokens = tokenizer_.tokenize(doc.content);
    doc.length = static_cast<uint32_t>(tokens.size());

    // Aggregate tokens by term: term -> (frequency, positions)
    std::unordered_map<std::string, std::pair<uint32_t, std::vector<Position>>> term_data;
    for (const auto& tok : tokens) {
        auto& entry = term_data[tok.text];
        entry.first++;
        entry.second.push_back(tok.position);
    }
    doc.unique_terms = static_cast<uint32_t>(term_data.size());

    // Insert into inverted index postings
    for (auto& [term, data] : term_data) {
        auto& postings = index_[term];
        // Check if doc already in postings list (e.g. re-index)
        auto p_it = std::lower_bound(postings.begin(), postings.end(), doc.id,
            [](const Posting& p, DocId id) { return p.doc_id < id; });
        
        if (p_it != postings.end() && p_it->doc_id == doc.id) {
            p_it->term_frequency = data.first;
            p_it->positions = std::move(data.second);
        } else {
            postings.insert(p_it, Posting{
                .doc_id = doc.id,
                .term_frequency = data.first,
                .positions = std::move(data.second)
            });
        }
    }

    total_tokens_ += doc.length;
    documents_[doc.id] = std::move(doc);
    return true;
}

bool InvertedIndex::delete_document(DocId id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = documents_.find(id);
    if (it == documents_.end() || deleted_docs_.find(id) != deleted_docs_.end()) {
        return false;
    }

    deleted_docs_.insert(id);
    if (total_tokens_ >= it->second.length) {
        total_tokens_ -= it->second.length;
    }
    return true;
}

bool InvertedIndex::update_document(Document doc) {
    return add_document(std::move(doc));
}

PostingList InvertedIndex::get_postings(const std::string& term) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = index_.find(term);
    if (it == index_.end()) {
        return {};
    }

    if (deleted_docs_.empty()) {
        return it->second;
    }

    PostingList active_postings;
    active_postings.reserve(it->second.size());
    for (const auto& p : it->second) {
        if (deleted_docs_.find(p.doc_id) == deleted_docs_.end()) {
            active_postings.push_back(p);
        }
    }
    return active_postings;
}

size_t InvertedIndex::get_doc_frequency(const std::string& term) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = index_.find(term);
    if (it == index_.end()) return 0;

    if (deleted_docs_.empty()) {
        return it->second.size();
    }

    size_t count = 0;
    for (const auto& p : it->second) {
        if (deleted_docs_.find(p.doc_id) == deleted_docs_.end()) {
            count++;
        }
    }
    return count;
}

bool InvertedIndex::has_document(DocId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return documents_.find(id) != documents_.end() && deleted_docs_.find(id) == deleted_docs_.end();
}

bool InvertedIndex::is_deleted(DocId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return deleted_docs_.find(id) != deleted_docs_.end();
}

std::optional<DocumentMeta> InvertedIndex::get_document_meta(DocId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = documents_.find(id);
    if (it == documents_.end() || deleted_docs_.find(id) != deleted_docs_.end()) {
        return std::nullopt;
    }
    return DocumentMeta{
        .id = it->second.id,
        .title = it->second.title,
        .source = it->second.source,
        .date = it->second.date,
        .doc_type = it->second.doc_type,
        .length = it->second.length
    };
}

std::optional<Document> InvertedIndex::get_document(DocId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = documents_.find(id);
    if (it == documents_.end() || deleted_docs_.find(id) != deleted_docs_.end()) {
        return std::nullopt;
    }
    return it->second;
}

size_t InvertedIndex::total_documents() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return documents_.size();
}

size_t InvertedIndex::active_documents() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return documents_.size() - deleted_docs_.size();
}

size_t InvertedIndex::deleted_documents() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return deleted_docs_.size();
}

size_t InvertedIndex::total_unique_terms() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return index_.size();
}

size_t InvertedIndex::total_token_count() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return total_tokens_;
}

double InvertedIndex::avg_doc_length() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    size_t active = documents_.size() - deleted_docs_.size();
    if (active == 0) return 0.0;
    return static_cast<double>(total_tokens_) / static_cast<double>(active);
}

IndexStats InvertedIndex::get_stats() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    size_t active = documents_.size() - deleted_docs_.size();
    double avg_len = active > 0 ? static_cast<double>(total_tokens_) / static_cast<double>(active) : 0.0;

    size_t total_postings = 0;
    size_t total_occurrences = 0;
    size_t est_bytes = sizeof(InvertedIndex);

    for (const auto& [term, postings] : index_) {
        est_bytes += sizeof(term) + term.capacity();
        est_bytes += sizeof(postings) + postings.capacity() * sizeof(Posting);
        for (const auto& p : postings) {
            total_postings++;
            total_occurrences += p.term_frequency;
            est_bytes += p.positions.capacity() * sizeof(Position);
        }
    }

    for (const auto& [id, doc] : documents_) {
        est_bytes += sizeof(DocId) + sizeof(Document);
        est_bytes += doc.title.capacity() + doc.source.capacity() + doc.date.capacity() + doc.doc_type.capacity() + doc.content.capacity();
    }
    est_bytes += deleted_docs_.size() * sizeof(DocId);

    return IndexStats{
        .total_documents = documents_.size(),
        .active_documents = active,
        .deleted_documents = deleted_docs_.size(),
        .total_unique_terms = index_.size(),
        .total_postings = total_postings,
        .total_token_occurrences = total_occurrences,
        .avg_doc_length = avg_len,
        .estimated_memory_bytes = est_bytes,
        .cache_entries = 0,
        .cache_hits = 0,
        .cache_misses = 0
    };
}

void InvertedIndex::compact() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (deleted_docs_.empty()) return;

    for (auto it = index_.begin(); it != index_.end(); ) {
        auto& postings = it->second;
        postings.erase(
            std::remove_if(postings.begin(), postings.end(),
                [this](const Posting& p) {
                    return deleted_docs_.find(p.doc_id) != deleted_docs_.end();
                }),
            postings.end()
        );

        if (postings.empty()) {
            it = index_.erase(it);
        } else {
            ++it;
        }
    }

    for (DocId id : deleted_docs_) {
        documents_.erase(id);
    }
    deleted_docs_.clear();
}

void InvertedIndex::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.clear();
    documents_.clear();
    deleted_docs_.clear();
    total_tokens_ = 0;
}

void InvertedIndex::restore_state(
    std::unordered_map<std::string, PostingList> index,
    std::unordered_map<DocId, Document> documents,
    std::unordered_set<DocId> deleted_docs,
    size_t total_tokens) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_ = std::move(index);
    documents_ = std::move(documents);
    deleted_docs_ = std::move(deleted_docs);
    total_tokens_ = total_tokens;
}

} // namespace indexforge
