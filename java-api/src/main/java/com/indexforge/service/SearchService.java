package com.indexforge.service;

import com.indexforge.exception.InvalidQueryException;
import com.indexforge.grpc.SearchHit;
import com.indexforge.grpc.SearchResponse;
import com.indexforge.model.SearchHistoryRecord;
import com.indexforge.model.dto.SearchHitDto;
import com.indexforge.model.dto.SearchResponseDto;
import com.indexforge.repository.SearchHistoryRepository;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.List;

@Service
public class SearchService {

    private final GrpcSearchEngineClient grpcClient;
    private final SearchHistoryRepository historyRepository;

    public SearchService(GrpcSearchEngineClient grpcClient, SearchHistoryRepository historyRepository) {
        this.grpcClient = grpcClient;
        this.historyRepository = historyRepository;
    }

    public SearchResponseDto executeSearch(String query, String algo, float k1, float b, int limit, int offset, String docType) {
        if (query == null || query.trim().isEmpty()) {
            throw new InvalidQueryException("Search query cannot be empty");
        }

        long startNanos = System.nanoTime();
        SearchResponse response = grpcClient.search(query.trim(), algo, k1, b, limit, offset, docType);
        double totalLatencyMs = (System.nanoTime() - startNanos) / 1_000_000.0;

        // Record in search history
        historyRepository.recordSearch(query, algo, response.getTotalHits(), totalLatencyMs);

        List<SearchHitDto> hits = new ArrayList<>();
        for (SearchHit hit : response.getHitsList()) {
            hits.add(new SearchHitDto(
                    hit.getDocId(),
                    hit.getTitle(),
                    hit.getSource(),
                    hit.getDate(),
                    hit.getDocType(),
                    hit.getScore(),
                    hit.getLength(),
                    hit.getSnippet(),
                    hit.getMatchedTermsList()
            ));
        }

        return new SearchResponseDto(
                response.getQuery(),
                response.getAlgorithm(),
                response.getTotalHits(),
                response.getEngineLatencyMs(),
                Math.round(totalLatencyMs * 1000.0) / 1000.0,
                response.getFromCache(),
                hits
        );
    }

    public List<SearchHistoryRecord> getRecentHistory(int limit) {
        return historyRepository.getRecentSearches(limit > 0 ? limit : 20);
    }

    public void clearHistory() {
        historyRepository.clearHistory();
    }
}
