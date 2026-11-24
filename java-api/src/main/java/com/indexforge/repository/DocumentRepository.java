package com.indexforge.repository;

import com.indexforge.model.DocumentMetadata;
import org.springframework.stereotype.Repository;

import java.sql.*;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

@Repository
public class DocumentRepository {

    private final DatabaseInitializer databaseInitializer;

    public DocumentRepository(DatabaseInitializer databaseInitializer) {
        this.databaseInitializer = databaseInitializer;
    }

    private Connection getConnection() throws SQLException {
        return DriverManager.getConnection(databaseInitializer.getJdbcUrl());
    }

    public synchronized int getNextDocId() {
        String sql = "SELECT COALESCE(MAX(id), 0) + 1 FROM documents";
        try (Connection conn = getConnection();
             Statement stmt = conn.createStatement();
             ResultSet rs = stmt.executeQuery(sql)) {
            if (rs.next()) {
                return rs.getInt(1);
            }
            return 1;
        } catch (SQLException e) {
            throw new RuntimeException("Error fetching next doc ID", e);
        }
    }

    public void insertDocument(int id, String title, String source, String date, String docType,
                               String fileName, String content, int length) {
        String sql = """
            INSERT OR REPLACE INTO documents (id, title, source, date, doc_type, file_name, content, length, indexed_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
        """;
        try (Connection conn = getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setInt(1, id);
            ps.setString(2, title);
            ps.setString(3, source);
            ps.setString(4, date);
            ps.setString(5, docType);
            ps.setString(6, fileName);
            ps.setString(7, content);
            ps.setInt(8, length);
            ps.setString(9, LocalDateTime.now().toString());
            ps.executeUpdate();
        } catch (SQLException e) {
            throw new RuntimeException("Error inserting document #" + id, e);
        }
    }

    public Optional<DocumentMetadata> findById(int id) {
        String sql = "SELECT id, title, source, date, doc_type, file_name, content, length, indexed_at FROM documents WHERE id = ?";
        try (Connection conn = getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setInt(1, id);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    return Optional.of(new DocumentMetadata(
                        rs.getInt("id"),
                        rs.getString("title"),
                        rs.getString("source"),
                        rs.getString("date"),
                        rs.getString("doc_type"),
                        rs.getString("file_name"),
                        rs.getInt("length"),
                        rs.getString("content"),
                        rs.getString("indexed_at")
                    ));
                }
            }
        } catch (SQLException e) {
            throw new RuntimeException("Error fetching document #" + id, e);
        }
        return Optional.empty();
    }

    public List<DocumentMetadata> listDocuments(String docType, int limit, int offset) {
        List<DocumentMetadata> list = new ArrayList<>();
        StringBuilder sql = new StringBuilder("SELECT id, title, source, date, doc_type, file_name, length, indexed_at FROM documents ");
        if (docType != null && !docType.isBlank() && !"all".equalsIgnoreCase(docType)) {
            sql.append("WHERE LOWER(doc_type) = LOWER(?) ");
        }
        sql.append("ORDER BY id ASC LIMIT ? OFFSET ?");

        try (Connection conn = getConnection();
             PreparedStatement ps = conn.prepareStatement(sql.toString())) {
            int pIdx = 1;
            if (docType != null && !docType.isBlank() && !"all".equalsIgnoreCase(docType)) {
                ps.setString(pIdx++, docType);
            }
            ps.setInt(pIdx++, limit);
            ps.setInt(pIdx, offset);

            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) {
                    list.add(new DocumentMetadata(
                        rs.getInt("id"),
                        rs.getString("title"),
                        rs.getString("source"),
                        rs.getString("date"),
                        rs.getString("doc_type"),
                        rs.getString("file_name"),
                        rs.getInt("length"),
                        null, // omit content for listing
                        rs.getString("indexed_at")
                    ));
                }
            }
        } catch (SQLException e) {
            throw new RuntimeException("Error listing documents", e);
        }
        return list;
    }

    public boolean deleteById(int id) {
        String sql = "DELETE FROM documents WHERE id = ?";
        try (Connection conn = getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setInt(1, id);
            return ps.executeUpdate() > 0;
        } catch (SQLException e) {
            throw new RuntimeException("Error deleting document #" + id, e);
        }
    }

    public int count(String docType) {
        StringBuilder sql = new StringBuilder("SELECT COUNT(*) FROM documents ");
        if (docType != null && !docType.isBlank() && !"all".equalsIgnoreCase(docType)) {
            sql.append("WHERE LOWER(doc_type) = LOWER(?)");
        }
        try (Connection conn = getConnection();
             PreparedStatement ps = conn.prepareStatement(sql.toString())) {
            if (docType != null && !docType.isBlank() && !"all".equalsIgnoreCase(docType)) {
                ps.setString(1, docType);
            }
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    return rs.getInt(1);
                }
            }
        } catch (SQLException e) {
            throw new RuntimeException("Error counting documents", e);
        }
        return 0;
    }

    public List<String> getDistinctDocTypes() {
        List<String> types = new ArrayList<>();
        String sql = "SELECT DISTINCT doc_type FROM documents WHERE doc_type IS NOT NULL ORDER BY doc_type ASC";
        try (Connection conn = getConnection();
             Statement stmt = conn.createStatement();
             ResultSet rs = stmt.executeQuery(sql)) {
            while (rs.next()) {
                types.add(rs.getString(1));
            }
        } catch (SQLException e) {
            throw new RuntimeException("Error fetching doc types", e);
        }
        return types;
    }
}
