package com.indexforge.controller;

import com.indexforge.config.SecurityConfig;
import com.indexforge.model.DocumentMetadata;
import com.indexforge.model.dto.IngestResponseDto;
import com.indexforge.model.dto.IngestTextRequestDto;
import com.indexforge.service.DocumentService;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Import;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;

import java.util.List;
import java.util.Optional;

import static org.mockito.ArgumentMatchers.*;
import static org.mockito.BDDMockito.given;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.*;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;

@WebMvcTest(controllers = DocumentController.class)
@Import(SecurityConfig.class)
class DocumentControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private ObjectMapper objectMapper;

    @MockBean
    private DocumentService documentService;

    @Test
    @DisplayName("GET /api/documents should return paginated list")
    void testListDocuments() throws Exception {
        DocumentMetadata doc = new DocumentMetadata(1, "Policy Document", "Gov Source", "2024-01-01", "policy", "doc.txt", 100, "Content", "2024-01-01T00:00:00");
        given(documentService.listDocuments(any(), anyInt(), anyInt())).willReturn(List.of(doc));
        given(documentService.countDocuments(any())).willReturn(1);

        mockMvc.perform(get("/api/documents")
                        .accept(MediaType.APPLICATION_JSON))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.total").value(1))
                .andExpect(jsonPath("$.documents[0].id").value(1))
                .andExpect(jsonPath("$.documents[0].title").value("Policy Document"));
    }

    @Test
    @DisplayName("GET /api/documents/{id} should return document when found")
    void testGetDocumentFound() throws Exception {
        DocumentMetadata doc = new DocumentMetadata(42, "Tender Spec", "DoD", "2024-02-01", "tender", "tender.txt", 250, "Detailed specifications", "2024-02-01T10:00:00");
        given(documentService.getDocument(42)).willReturn(Optional.of(doc));

        mockMvc.perform(get("/api/documents/42"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.id").value(42))
                .andExpect(jsonPath("$.title").value("Tender Spec"));
    }

    @Test
    @DisplayName("GET /api/documents/{id} should return 404 when document not found")
    void testGetDocumentNotFound() throws Exception {
        given(documentService.getDocument(999)).willReturn(Optional.empty());

        mockMvc.perform(get("/api/documents/999"))
                .andExpect(status().isNotFound());
    }

    @Test
    @DisplayName("POST /api/ingest/text should ingest document and return 201 Created")
    void testIngestDocument() throws Exception {
        IngestTextRequestDto request = new IngestTextRequestDto("New Policy", "Internal Audit", "2024-03-01", "policy", "Full policy text content.");
        IngestResponseDto response = new IngestResponseDto(true, 101, "New Policy", 50, "Document ingested and indexed successfully");

        given(documentService.ingestDocument(any(IngestTextRequestDto.class))).willReturn(response);

        mockMvc.perform(post("/api/ingest/text")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content(objectMapper.writeValueAsString(request)))
                .andExpect(status().isCreated())
                .andExpect(jsonPath("$.success").value(true))
                .andExpect(jsonPath("$.docId").value(101));
    }
}
