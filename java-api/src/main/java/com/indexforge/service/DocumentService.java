package com.indexforge.service;

import com.indexforge.grpc.IndexDocumentResponse;
import com.indexforge.model.DocumentMetadata;
import com.indexforge.model.dto.IngestResponseDto;
import com.indexforge.model.dto.IngestTextRequestDto;
import com.indexforge.repository.DocumentRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@Service
public class DocumentService {

    private static final Logger log = LoggerFactory.getLogger(DocumentService.class);

    private final DocumentRepository documentRepository;
    private final GrpcSearchEngineClient grpcClient;

    @Value("${indexforge.storage.sample-docs-dir:../storage/sample_docs}")
    private String sampleDocsDir;

    public DocumentService(DocumentRepository documentRepository, GrpcSearchEngineClient grpcClient) {
        this.documentRepository = documentRepository;
        this.grpcClient = grpcClient;
    }

    public IngestResponseDto ingestDocument(IngestTextRequestDto req) {
        int docId = documentRepository.getNextDocId();
        int tokenLength = req.content().split("\\s+").length;

        String title = req.title() != null && !req.title().isBlank() ? req.title() : "Document #" + docId;
        String source = req.source() != null ? req.source() : "Manual Submission";
        String date = req.date() != null ? req.date() : "2026-01-01";
        String docType = req.doc_type() != null ? req.doc_type() : "policy";

        // 1. Store in SQLite metadata store
        documentRepository.insertDocument(docId, title, source, date, docType, "", req.content(), tokenLength);

        // 2. Index in C++ Engine via gRPC
        IndexDocumentResponse resp = grpcClient.indexDocument(docId, title, source, date, docType, req.content(), "");

        return new IngestResponseDto(
                resp.getSuccess(),
                docId,
                title,
                tokenLength,
                resp.getSuccess() ? "Document successfully indexed in C++ engine" : resp.getErrorMessage()
        );
    }

    public List<DocumentMetadata> listDocuments(String docType, int limit, int offset) {
        return documentRepository.listDocuments(docType, limit > 0 ? limit : 20, offset >= 0 ? offset : 0);
    }

    public Optional<DocumentMetadata> getDocument(int docId) {
        return documentRepository.findById(docId);
    }

    public boolean deleteDocument(int docId) {
        boolean dbDeleted = documentRepository.deleteById(docId);
        boolean engineDeleted = false;
        try {
            engineDeleted = grpcClient.deleteDocument(docId).getSuccess();
        } catch (Exception e) {
            log.warn("C++ Engine delete warning for doc #{}: {}", docId, e.getMessage());
        }
        return dbDeleted || engineDeleted;
    }

    public int countDocuments(String docType) {
        return documentRepository.count(docType);
    }

    public List<String> getAvailableDocTypes() {
        return documentRepository.getDistinctDocTypes();
    }

    public List<IngestResponseDto> ingestSampleDirectory() {
        List<IngestResponseDto> results = new ArrayList<>();
        File dir = new File(sampleDocsDir);
        if (!dir.exists() || !dir.isDirectory()) {
            dir = new File("../data/sample_docs");
        }
        if (!dir.exists() || !dir.isDirectory()) {
            log.warn("Sample documents directory not found at {}", sampleDocsDir);
            return results;
        }

        File[] files = dir.listFiles((d, name) -> name.endsWith(".txt") || name.endsWith(".md"));
        if (files == null) return results;

        for (File f : files) {
            try {
                String content = Files.readString(f.toPath());
                String title = extractTitle(content, f.getName());
                String source = extractPattern(content, "(?:\\*\\*Source:\\*\\*|Source:|Agency:|AGENCY:)\\s*(.+)", "Federal Repository");
                String date = extractPattern(content, "(?:\\*\\*Date:\\*\\*|Date:|DATE:)\\s*([0-9]{4}-[0-9]{2}-[0-9]{2})", "2026-06-01");
                String docType = extractDocType(content, f.getName());

                int docId = documentRepository.getNextDocId();
                int tokenLength = content.split("\\s+").length;

                documentRepository.insertDocument(docId, title, source, date, docType, f.getName(), content, tokenLength);
                IndexDocumentResponse resp = grpcClient.indexDocument(docId, title, source, date, docType, content, f.getName());

                results.add(new IngestResponseDto(resp.getSuccess(), docId, title, tokenLength, "Sample loaded"));
            } catch (IOException e) {
                log.error("Failed to read sample file {}: {}", f.getName(), e.getMessage());
            }
        }
        return results;
    }

    private String extractTitle(String content, String filename) {
        Pattern p = Pattern.compile("^(?:#\\s*|Title:\\s*)(.+)$", Pattern.MULTILINE);
        Matcher m = p.matcher(content);
        if (m.find()) {
            return m.group(1).trim();
        }
        return filename.replace(".txt", "").replace(".md", "").replace("_", " ").toUpperCase();
    }

    private String extractPattern(String content, String regex, String defaultVal) {
        Pattern p = Pattern.compile(regex, Pattern.CASE_INSENSITIVE);
        Matcher m = p.matcher(content);
        if (m.find()) {
            return m.group(1).trim();
        }
        return defaultVal;
    }

    private String extractDocType(String content, String filename) {
        String lowerName = filename.toLowerCase();
        if (lowerName.contains("tender")) return "tender";
        if (lowerName.contains("notice")) return "notice";
        if (lowerName.contains("regulation") || lowerName.contains("sp800") || lowerName.contains("rmf")) return "regulation";
        if (lowerName.contains("tech") || lowerName.contains("strategy") || lowerName.contains("transit")) return "tech_doc";
        return "policy";
    }
}
