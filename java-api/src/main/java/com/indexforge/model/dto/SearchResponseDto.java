package com.indexforge.model.dto;

import java.util.List;

public record SearchResponseDto(
    String query,
    String algorithm,
    int totalHits,
    double engineLatencyMs,
    double totalLatencyMs,
    boolean fromCache,
    List<SearchHitDto> hits
) {}
