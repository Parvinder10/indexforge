package com.indexforge.model.dto;

public record IngestResponseDto(
    boolean success,
    int docId,
    String title,
    int tokensIndexed,
    String message
) {}
