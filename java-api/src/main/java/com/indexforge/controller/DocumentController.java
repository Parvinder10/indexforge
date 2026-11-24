package com.indexforge.controller;

import com.indexforge.model.DocumentMetadata;
import com.indexforge.model.dto.IngestResponseDto;
import com.indexforge.model.dto.IngestTextRequestDto;
import com.indexforge.service.DocumentService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RestController
@Tag(name = "Documents", description = "Document Ingestion, Metadata Management, and Retrieval")
public class DocumentController {

    private final DocumentService documentService;

    public DocumentController(DocumentService documentService) {
        this.documentService = documentService;
    }

    @GetMapping("/api/documents")
    @Operation(summary = "List documents with metadata", description = "Retrieves paginated list of documents from SQLite storage")
    public ResponseEntity<Map<String, Object>> listDocuments(
            @RequestParam(required = false) String doc_type,
            @RequestParam(defaultValue = "20") int limit,
            @RequestParam(defaultValue = "0") int offset
    ) {
        List<DocumentMetadata> docs = documentService.listDocuments(doc_type, limit, offset);
        int total = documentService.countDocuments(doc_type);

        Map<String, Object> resp = new HashMap<>();
        resp.put("total", total);
        resp.put("limit", limit);
        resp.put("offset", offset);
        resp.put("documents", docs);
        return ResponseEntity.ok(resp);
    }

    @GetMapping("/api/documents/{id}")
    @Operation(summary = "Get full document details", description = "Retrieves full text and metadata for a specific document ID")
    public ResponseEntity<DocumentMetadata> getDocument(@PathVariable int id) {
        return documentService.getDocument(id)
                .map(ResponseEntity::ok)
                .orElse(ResponseEntity.notFound().build());
    }

    @DeleteMapping("/api/documents/{id}")
    @Operation(summary = "Delete document", description = "Deletes document atomically from SQLite and removes it from C++ inverted index via gRPC")
    public ResponseEntity<Map<String, Object>> deleteDocument(@PathVariable int id) {
        boolean ok = documentService.deleteDocument(id);
        Map<String, Object> resp = new HashMap<>();
        resp.put("success", ok);
        resp.put("doc_id", id);
        return ResponseEntity.ok(resp);
    }

    @PostMapping({"/api/documents/ingest/text", "/api/ingest/text"})
    @Operation(summary = "Ingest text document", description = "Stores metadata in SQLite and indexes tokens in C++ engine via gRPC")
    public ResponseEntity<IngestResponseDto> ingestText(@Valid @RequestBody IngestTextRequestDto request) {
        IngestResponseDto result = documentService.ingestDocument(request);
        return ResponseEntity.status(result.success() ? HttpStatus.CREATED : HttpStatus.BAD_REQUEST).body(result);
    }

    @PostMapping({"/api/documents/ingest/sample", "/api/ingest/sample"})
    @Operation(summary = "Import built-in sample collection", description = "Indexes federal regulations, public tenders, and policy documents from storage/sample_docs")
    public ResponseEntity<Map<String, Object>> ingestSampleDocuments() {
        List<IngestResponseDto> list = documentService.ingestSampleDirectory();
        Map<String, Object> resp = new HashMap<>();
        resp.put("status", "success");
        resp.put("ingested_count", list.size());
        resp.put("documents", list);
        return ResponseEntity.ok(resp);
    }
}
