package com.indexforge.model.dto;

public record HealthDto(
    String status,
    String application,
    String grpcStatus,
    String engineInfo,
    long uptimeSeconds,
    long timestamp
) {}
