package com.indexforge.model.dto;

import java.util.List;

public record SearchHitDto(
    int docId,
    String title,
    String source,
    String date,
    String docType,
    double score,
    int length,
    String snippet,
    List<String> matchedTerms
) {}
