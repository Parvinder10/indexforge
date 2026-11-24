package com.indexforge.controller;

import com.indexforge.model.SearchHistoryRecord;
import com.indexforge.model.dto.SearchResponseDto;
import com.indexforge.service.SearchService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.Parameter;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/search")
@Tag(name = "Search", description = "Query the C++ Inverted Index Core using BM25, TF-IDF, or Boolean expressions")
public class SearchController {

    private final SearchService searchService;

    public SearchController(SearchService searchService) {
        this.searchService = searchService;
    }

    @GetMapping
    @Operation(summary = "Execute full-text search", description = "Dispatches search to C++ engine via gRPC and records query in search history")
    public ResponseEntity<SearchResponseDto> search(
            @Parameter(description = "Keywords, Boolean expression (AND, OR, NOT), or exact \"phrase\"")
            @RequestParam String q,
            @Parameter(description = "Ranking algorithm: bm25, tfidf, or linear")
            @RequestParam(defaultValue = "bm25") String algo,
            @Parameter(description = "BM25 k1 parameter (term frequency saturation)")
            @RequestParam(defaultValue = "1.2") float k1,
            @Parameter(description = "BM25 b parameter (document length normalization)")
            @RequestParam(defaultValue = "0.75") float b,
            @Parameter(description = "Filter by document type (tender, policy, regulation, notice, tech_doc)")
            @RequestParam(required = false) String doc_type,
            @Parameter(description = "Max results to return")
            @RequestParam(defaultValue = "20") int limit,
            @Parameter(description = "Pagination offset")
            @RequestParam(defaultValue = "0") int offset
    ) {
        SearchResponseDto result = searchService.executeSearch(q, algo, k1, b, limit, offset, doc_type);
        return ResponseEntity.ok(result);
    }

    @GetMapping("/linear")
    @Operation(summary = "Execute unindexed baseline linear scan", description = "Direct sequential file scan for empirical comparison against inverted index")
    public ResponseEntity<SearchResponseDto> searchLinear(
            @RequestParam String q,
            @RequestParam(defaultValue = "20") int limit
    ) {
        SearchResponseDto result = searchService.executeSearch(q, "linear", 1.2f, 0.75f, limit, 0, null);
        return ResponseEntity.ok(result);
    }

    @GetMapping("/history")
    @Operation(summary = "Retrieve recent search history", description = "Returns recent search queries, hit counts, and response latencies")
    public ResponseEntity<List<SearchHistoryRecord>> getHistory(
            @RequestParam(defaultValue = "20") int limit
    ) {
        return ResponseEntity.ok(searchService.getRecentHistory(limit));
    }

    @DeleteMapping("/history")
    @Operation(summary = "Clear search history", description = "Purges search query audit records from SQLite")
    public ResponseEntity<Map<String, String>> clearHistory() {
        searchService.clearHistory();
        return ResponseEntity.ok(Map.of("status", "success", "message", "Search history cleared"));
    }
}
