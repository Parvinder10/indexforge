package com.indexforge.model.dto;

import java.util.List;

public record IndexStatsDto(
    int total_documents,
    int active_documents,
    int deleted_documents,
    int total_unique_terms,
    int total_postings,
    int total_token_occurrences,
    double avg_doc_length,
    long estimated_memory_bytes,
    int cache_entries,
    int cache_hits,
    int cache_misses,
    int sqlite_documents,
    List<String> available_doc_types,
    String engine_version
) {}
