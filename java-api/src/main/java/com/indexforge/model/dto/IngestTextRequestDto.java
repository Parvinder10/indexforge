package com.indexforge.model.dto;

import jakarta.validation.constraints.NotBlank;

public record IngestTextRequestDto(
    @NotBlank(message = "Title is required")
    String title,
    String source,
    String date,
    String doc_type,
    @NotBlank(message = "Content is required")
    String content
) {}
