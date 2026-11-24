package com.indexforge.service;

import com.indexforge.exception.EngineUnavailableException;
import com.indexforge.grpc.*;
import io.grpc.StatusRuntimeException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.util.concurrent.TimeUnit;

@Service
public class GrpcSearchEngineClient {

    private static final Logger log = LoggerFactory.getLogger(GrpcSearchEngineClient.class);

    private final SearchEngineServiceGrpc.SearchEngineServiceBlockingStub blockingStub;

    @Value("${indexforge.grpc.timeout-seconds:5}")
    private int timeoutSeconds;

    public GrpcSearchEngineClient(SearchEngineServiceGrpc.SearchEngineServiceBlockingStub blockingStub) {
        this.blockingStub = blockingStub;
    }

    private SearchEngineServiceGrpc.SearchEngineServiceBlockingStub stubWithTimeout() {
        return blockingStub.withDeadlineAfter(timeoutSeconds, TimeUnit.SECONDS);
    }

    public SearchResponse search(String query, String algo, float k1, float b, int limit, int offset, String docType) {
        RankingAlgorithm rankingAlgorithm = switch (algo.toLowerCase()) {
            case "tfidf", "tf_idf" -> RankingAlgorithm.TF_IDF;
            case "linear", "linear_scan" -> RankingAlgorithm.LINEAR_SCAN;
            default -> RankingAlgorithm.BM25;
        };

        SearchRequest request = SearchRequest.newBuilder()
                .setQuery(query)
                .setAlgorithm(rankingAlgorithm)
                .setK1(k1)
                .setB(b)
                .setLimit(limit)
                .setOffset(offset)
                .setDocType(docType != null ? docType : "")
                .build();

        try {
            return stubWithTimeout().search(request);
        } catch (StatusRuntimeException e) {
            log.error("gRPC search failed for query [{}]: {}", query, e.getMessage());
            throw new EngineUnavailableException("Failed to execute search in C++ engine: " + e.getMessage(), e);
        }
    }

    public IndexDocumentResponse indexDocument(int docId, String title, String source, String date,
                                               String docType, String content, String fileName) {
        IndexDocumentRequest request = IndexDocumentRequest.newBuilder()
                .setDocId(docId)
                .setTitle(title != null ? title : "")
                .setSource(source != null ? source : "")
                .setDate(date != null ? date : "")
                .setDocType(docType != null ? docType : "policy")
                .setContent(content)
                .setFileName(fileName != null ? fileName : "")
                .build();

        try {
            return stubWithTimeout().indexDocument(request);
        } catch (StatusRuntimeException e) {
            log.error("gRPC indexing failed for doc #{}: {}", docId, e.getMessage());
            throw new EngineUnavailableException("Failed to index document in C++ engine: " + e.getMessage(), e);
        }
    }

    public DeleteDocumentResponse deleteDocument(int docId) {
        DeleteDocumentRequest request = DeleteDocumentRequest.newBuilder()
                .setDocId(docId)
                .build();
        try {
            return stubWithTimeout().deleteDocument(request);
        } catch (StatusRuntimeException e) {
            log.error("gRPC delete failed for doc #{}: {}", docId, e.getMessage());
            throw new EngineUnavailableException("Failed to delete document in C++ engine: " + e.getMessage(), e);
        }
    }

    public StatsResponse getStats() {
        try {
            return stubWithTimeout().getStats(StatsRequest.getDefaultInstance());
        } catch (StatusRuntimeException e) {
            log.warn("gRPC getStats failed: {}", e.getMessage());
            throw new EngineUnavailableException("C++ search engine stats unavailable: " + e.getMessage(), e);
        }
    }

    public HealthResponse checkHealth() {
        try {
            return stubWithTimeout().checkHealth(HealthRequest.getDefaultInstance());
        } catch (StatusRuntimeException e) {
            log.warn("gRPC health check failed: {}", e.getMessage());
            return HealthResponse.newBuilder()
                    .setStatus("NOT_SERVING")
                    .setEngineInfo("Engine unreachable: " + e.getMessage())
                    .setUptimeSeconds(0)
                    .build();
        }
    }

    public SaveIndexResponse saveIndex(String filepath) {
        SaveIndexRequest request = SaveIndexRequest.newBuilder().setFilepath(filepath).build();
        try {
            return stubWithTimeout().saveIndex(request);
        } catch (StatusRuntimeException e) {
            throw new EngineUnavailableException("Save index failed: " + e.getMessage(), e);
        }
    }

    public LoadIndexResponse loadIndex(String filepath) {
        LoadIndexRequest request = LoadIndexRequest.newBuilder().setFilepath(filepath).build();
        try {
            return stubWithTimeout().loadIndex(request);
        } catch (StatusRuntimeException e) {
            throw new EngineUnavailableException("Load index failed: " + e.getMessage(), e);
        }
    }

    public VerifyIntegrityResponse verifyIntegrity(String filepath) {
        VerifyIntegrityRequest request = VerifyIntegrityRequest.newBuilder().setFilepath(filepath).build();
        try {
            return stubWithTimeout().verifyIntegrity(request);
        } catch (StatusRuntimeException e) {
            throw new EngineUnavailableException("Verify integrity failed: " + e.getMessage(), e);
        }
    }

    public ClearCacheResponse clearCache() {
        try {
            return stubWithTimeout().clearCache(ClearCacheRequest.getDefaultInstance());
        } catch (StatusRuntimeException e) {
            throw new EngineUnavailableException("Clear cache failed: " + e.getMessage(), e);
        }
    }
}
