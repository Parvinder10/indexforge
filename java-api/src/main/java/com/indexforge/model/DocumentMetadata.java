package com.indexforge.model;

import java.time.LocalDateTime;

public record DocumentMetadata(
    int id,
    String title,
    String source,
    String date,
    String docType,
    String fileName,
    int length,
    String content,
    String indexedAt
) {}
