package com.indexforge.controller;

import com.indexforge.config.SecurityConfig;
import com.indexforge.model.dto.HealthDto;
import com.indexforge.model.dto.IndexStatsDto;
import com.indexforge.service.IndexManagementService;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Import;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;

import java.util.List;

import static org.mockito.BDDMockito.given;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;

@WebMvcTest(controllers = IndexController.class)
@Import(SecurityConfig.class)
class IndexControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockBean
    private IndexManagementService indexManagementService;

    @Test
    @DisplayName("GET /api/index/stats should return index statistics")
    void testGetStats() throws Exception {
        IndexStatsDto stats = new IndexStatsDto(
                12, 12, 0, 850, 2400, 12000, 1000.0,
                150000L, 4, 10, 2, 12,
                List.of("policy", "tender"), "IndexForge C++20 Core v1.0.0"
        );
        given(indexManagementService.getCombinedStats()).willReturn(stats);

        mockMvc.perform(get("/api/index/stats")
                        .accept(MediaType.APPLICATION_JSON))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.total_documents").value(12))
                .andExpect(jsonPath("$.total_unique_terms").value(850));
    }

    @Test
    @DisplayName("GET /api/index/health should return engine health status")
    void testGetHealth() throws Exception {
        HealthDto health = new HealthDto("healthy", "IndexForge Spring Boot 3", "SERVING", "C++20 Engine v1.0", 120L, System.currentTimeMillis());
        given(indexManagementService.checkHealth()).willReturn(health);

        mockMvc.perform(get("/api/index/health")
                        .accept(MediaType.APPLICATION_JSON))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.status").value("healthy"))
                .andExpect(jsonPath("$.grpcStatus").value("SERVING"));
    }
}
