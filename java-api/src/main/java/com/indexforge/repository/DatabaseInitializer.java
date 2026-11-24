package com.indexforge.repository;

import jakarta.annotation.PostConstruct;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.io.File;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.Statement;

@Component
public class DatabaseInitializer {

    private static final Logger log = LoggerFactory.getLogger(DatabaseInitializer.class);

    @Value("${indexforge.storage.database-path:../storage/metadata.db}")
    private String databasePath;

    @PostConstruct
    public void initializeDatabase() {
        File dbFile = new File(databasePath);
        File parentDir = dbFile.getParentFile();
        if (parentDir != null && !parentDir.exists()) {
            parentDir.mkdirs();
        }

        String url = "jdbc:sqlite:" + dbFile.getAbsolutePath();
        try (Connection conn = DriverManager.getConnection(url);
             Statement stmt = conn.createStatement()) {

            // Enable WAL mode for high concurrency
            stmt.execute("PRAGMA journal_mode = WAL;");
            stmt.execute("PRAGMA synchronous = NORMAL;");

            // Documents metadata table
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS documents (
                    id INTEGER PRIMARY KEY,
                    title TEXT NOT NULL,
                    source TEXT,
                    date TEXT,
                    doc_type TEXT DEFAULT 'policy',
                    file_name TEXT,
                    content TEXT NOT NULL,
                    length INTEGER DEFAULT 0,
                    indexed_at TEXT
                );
            """);

            stmt.execute("CREATE INDEX IF NOT EXISTS idx_doc_type ON documents(doc_type);");

            // Search history table
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS search_history (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    query TEXT NOT NULL,
                    algorithm TEXT NOT NULL,
                    total_hits INTEGER DEFAULT 0,
                    latency_ms REAL DEFAULT 0.0,
                    searched_at TEXT NOT NULL
                );
            """);

            stmt.execute("CREATE INDEX IF NOT EXISTS idx_searched_at ON search_history(searched_at DESC);");

            log.info("[✓] SQLite database initialized at {}", dbFile.getAbsolutePath());
        } catch (SQLException e) {
            log.error("Failed to initialize SQLite database: {}", e.getMessage(), e);
            throw new RuntimeException("SQLite initialization failed", e);
        }
    }

    public String getJdbcUrl() {
        return "jdbc:sqlite:" + new File(databasePath).getAbsolutePath();
    }
}
