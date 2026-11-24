package com.indexforge.model;

public record SearchHistoryRecord(
    int id,
    String query,
    String algorithm,
    int totalHits,
    double latencyMs,
    String searchedAt
) {}
