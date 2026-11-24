package com.indexforge.controller;

import com.indexforge.model.dto.HealthDto;
import com.indexforge.model.dto.IndexStatsDto;
import com.indexforge.service.IndexManagementService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

@RestController
@Tag(name = "Index Management", description = "Engine Telemetry, Statistics, Cache Control, and Integrity Verification")
public class IndexController {

    private final IndexManagementService indexService;

    public IndexController(IndexManagementService indexService) {
        this.indexService = indexService;
    }

    @GetMapping({"/api/index/stats", "/api/stats"})
    @Operation(summary = "Get index statistics", description = "Combines C++ inverted index metrics with SQLite corpus counts")
    public ResponseEntity<IndexStatsDto> getStats() {
        return ResponseEntity.ok(indexService.getCombinedStats());
    }

    @GetMapping({"/api/index/health", "/api/health"})
    @Operation(summary = "Service health check", description = "Verifies Spring Boot status and C++ engine gRPC connectivity")
    public ResponseEntity<HealthDto> getHealth() {
        return ResponseEntity.ok(indexService.checkHealth());
    }

    @PostMapping({"/api/index/cache/clear", "/api/cache/clear"})
    @Operation(summary = "Clear query result cache", description = "Purges in-memory LRU query cache in C++ engine")
    public ResponseEntity<Map<String, Object>> clearCache() {
        return ResponseEntity.ok(indexService.clearCache());
    }

    @PostMapping("/api/index/save")
    @Operation(summary = "Persist index to disk", description = "Serializes inverted index to binary .forge format with CRC32 checksum")
    public ResponseEntity<Map<String, Object>> saveIndex(@RequestParam(defaultValue = "index.forge") String filepath) {
        return ResponseEntity.ok(indexService.saveIndex(filepath));
    }

    @PostMapping("/api/index/load")
    @Operation(summary = "Reload index from disk", description = "Loads binary .forge file into C++ inverted index")
    public ResponseEntity<Map<String, Object>> loadIndex(@RequestParam(defaultValue = "index.forge") String filepath) {
        return ResponseEntity.ok(indexService.loadIndex(filepath));
    }

    @PostMapping("/api/index/verify")
    @Operation(summary = "Verify index integrity", description = "Validates magic header, format version, and IEEE 802.3 CRC32 checksum")
    public ResponseEntity<Map<String, Object>> verifyIntegrity(@RequestParam(defaultValue = "index.forge") String filepath) {
        return ResponseEntity.ok(indexService.verifyIntegrity(filepath));
    }
}
