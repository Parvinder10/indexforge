package com.indexforge.service;

import com.indexforge.grpc.HealthResponse;
import com.indexforge.grpc.StatsResponse;
import com.indexforge.grpc.VerifyIntegrityResponse;
import com.indexforge.model.dto.HealthDto;
import com.indexforge.model.dto.IndexStatsDto;
import com.indexforge.repository.DocumentRepository;
import org.springframework.stereotype.Service;

import java.util.HashMap;
import java.util.Map;

@Service
public class IndexManagementService {

    private final GrpcSearchEngineClient grpcClient;
    private final DocumentRepository documentRepository;

    public IndexManagementService(GrpcSearchEngineClient grpcClient, DocumentRepository documentRepository) {
        this.grpcClient = grpcClient;
        this.documentRepository = documentRepository;
    }

    public IndexStatsDto getCombinedStats() {
        StatsResponse grpcStats = grpcClient.getStats();
        int dbDocCount = documentRepository.count("all");

        return new IndexStatsDto(
                grpcStats.getTotalDocuments(),
                grpcStats.getActiveDocuments(),
                grpcStats.getDeletedDocuments(),
                grpcStats.getTotalUniqueTerms(),
                grpcStats.getTotalPostings(),
                grpcStats.getTotalTokenOccurrences(),
                grpcStats.getAvgDocLength(),
                grpcStats.getEstimatedMemoryBytes(),
                grpcStats.getCacheEntries(),
                grpcStats.getCacheHits(),
                grpcStats.getCacheMisses(),
                dbDocCount,
                documentRepository.getDistinctDocTypes(),
                grpcStats.getEngineVersion()
        );
    }

    public HealthDto checkHealth() {
        HealthResponse hr = grpcClient.checkHealth();
        return new HealthDto(
                "SERVING".equals(hr.getStatus()) ? "healthy" : "degraded",
                "IndexForge Spring Boot 3 + Java 21 Backend",
                hr.getStatus(),
                hr.getEngineInfo(),
                hr.getUptimeSeconds(),
                System.currentTimeMillis()
        );
    }

    public Map<String, Object> saveIndex(String filepath) {
        var resp = grpcClient.saveIndex(filepath != null ? filepath : "index.forge");
        Map<String, Object> map = new HashMap<>();
        map.put("status", resp.getSuccess() ? "saved" : "failed");
        map.put("filepath", resp.getFilepath());
        map.put("error", resp.getErrorMessage());
        return map;
    }

    public Map<String, Object> loadIndex(String filepath) {
        var resp = grpcClient.loadIndex(filepath != null ? filepath : "index.forge");
        Map<String, Object> map = new HashMap<>();
        map.put("status", resp.getSuccess() ? "loaded" : "failed");
        map.put("filepath", resp.getFilepath());
        map.put("documents_loaded", resp.getDocumentsLoaded());
        map.put("error", resp.getErrorMessage());
        return map;
    }

    public Map<String, Object> verifyIntegrity(String filepath) {
        VerifyIntegrityResponse resp = grpcClient.verifyIntegrity(filepath != null ? filepath : "index.forge");
        Map<String, Object> map = new HashMap<>();
        map.put("valid", resp.getValid());
        map.put("checksum", resp.getChecksum());
        map.put("bytes_checked", resp.getBytesChecked());
        map.put("error_message", resp.getErrorMessage());
        return map;
    }

    public Map<String, Object> clearCache() {
        grpcClient.clearCache();
        Map<String, Object> map = new HashMap<>();
        map.put("status", "cache cleared");
        return map;
    }
}
