package com.indexforge.repository;

import com.indexforge.model.SearchHistoryRecord;
import org.springframework.stereotype.Repository;

import java.sql.*;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;

@Repository
public class SearchHistoryRepository {

    private final DatabaseInitializer databaseInitializer;

    public SearchHistoryRepository(DatabaseInitializer databaseInitializer) {
        this.databaseInitializer = databaseInitializer;
    }

    private Connection getConnection() throws SQLException {
        return DriverManager.getConnection(databaseInitializer.getJdbcUrl());
    }

    public void recordSearch(String query, String algorithm, int totalHits, double latencyMs) {
        String sql = "INSERT INTO search_history (query, algorithm, total_hits, latency_ms, searched_at) VALUES (?, ?, ?, ?, ?)";
        try (Connection conn = getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setString(1, query);
            ps.setString(2, algorithm);
            ps.setInt(3, totalHits);
            ps.setDouble(4, latencyMs);
            ps.setString(5, LocalDateTime.now().toString());
            ps.executeUpdate();
        } catch (SQLException e) {
            // Non-critical logging
            System.err.println("Warning: Failed to record search history: " + e.getMessage());
        }
    }

    public List<SearchHistoryRecord> getRecentSearches(int limit) {
        List<SearchHistoryRecord> list = new ArrayList<>();
        String sql = "SELECT id, query, algorithm, total_hits, latency_ms, searched_at FROM search_history ORDER BY id DESC LIMIT ?";
        try (Connection conn = getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setInt(1, limit);
            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) {
                    list.add(new SearchHistoryRecord(
                        rs.getInt("id"),
                        rs.getString("query"),
                        rs.getString("algorithm"),
                        rs.getInt("total_hits"),
                        rs.getDouble("latency_ms"),
                        rs.getString("searched_at")
                    ));
                }
            }
        } catch (SQLException e) {
            throw new RuntimeException("Error fetching search history", e);
        }
        return list;
    }

    public void clearHistory() {
        String sql = "DELETE FROM search_history";
        try (Connection conn = getConnection();
             Statement stmt = conn.createStatement()) {
            stmt.executeUpdate(sql);
        } catch (SQLException e) {
            throw new RuntimeException("Error clearing search history", e);
        }
    }
}
