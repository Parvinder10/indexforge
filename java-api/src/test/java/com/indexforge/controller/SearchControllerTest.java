package com.indexforge.controller;

import com.indexforge.config.SecurityConfig;
import com.indexforge.model.SearchHistoryRecord;
import com.indexforge.model.dto.SearchHitDto;
import com.indexforge.model.dto.SearchResponseDto;
import com.indexforge.service.SearchService;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Import;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;

import java.util.List;

import static org.mockito.ArgumentMatchers.*;
import static org.mockito.BDDMockito.given;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.*;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;

@WebMvcTest(controllers = SearchController.class)
@Import(SecurityConfig.class)
class SearchControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockBean
    private SearchService searchService;

    @Test
    @DisplayName("GET /api/search should return matching search hits")
    void testSearchEndpoint() throws Exception {
        SearchHitDto hit = new SearchHitDto(1, "Sample Tender Doc", "DoD", "2024-01-01", "tender", 3.42, 120, "This is a tender snippet.", List.of("tender"));
        SearchResponseDto response = new SearchResponseDto("tender", "bm25", 1, 0.45, 1.2, false, List.of(hit));

        given(searchService.executeSearch(eq("tender"), anyString(), anyFloat(), anyFloat(), anyInt(), anyInt(), any()))
                .willReturn(response);

        mockMvc.perform(get("/api/search")
                        .param("q", "tender")
                        .param("algo", "bm25")
                        .accept(MediaType.APPLICATION_JSON))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.query").value("tender"))
                .andExpect(jsonPath("$.algorithm").value("bm25"))
                .andExpect(jsonPath("$.totalHits").value(1))
                .andExpect(jsonPath("$.hits[0].docId").value(1));
    }

    @Test
    @DisplayName("GET /api/search/history should return audit history")
    void testGetHistory() throws Exception {
        SearchHistoryRecord record = new SearchHistoryRecord(1, "procurement", "bm25", 5, 1.25f, "2024-01-01 12:00:00");
        given(searchService.getRecentHistory(anyInt())).willReturn(List.of(record));

        mockMvc.perform(get("/api/search/history")
                        .accept(MediaType.APPLICATION_JSON))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$[0].query").value("procurement"))
                .andExpect(jsonPath("$[0].totalHits").value(5));
    }

    @Test
    @DisplayName("DELETE /api/search/history should return success confirmation")
    void testClearHistory() throws Exception {
        mockMvc.perform(delete("/api/search/history"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.status").value("success"));
    }
}
