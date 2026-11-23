import sqlite3
import datetime
from pathlib import Path
from typing import Dict, Any, List, Optional

class MetadataDB:
    def __init__(self, db_path: Optional[str] = None):
        if db_path is None:
            base_dir = Path(__file__).resolve().parent.parent
            data_dir = base_dir / "data"
            data_dir.mkdir(exist_ok=True)
            db_path = str(data_dir / "metadata.db")

        self.db_path = db_path
        self._init_db()

    def _get_connection(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self.db_path, timeout=30.0)
        conn.row_factory = sqlite3.Row
        # Enable WAL mode for high concurrent performance
        conn.execute("PRAGMA journal_mode = WAL;")
        conn.execute("PRAGMA synchronous = NORMAL;")
        return conn

    def _init_db(self):
        with self._get_connection() as conn:
            conn.execute("""
                CREATE TABLE IF NOT EXISTS documents (
                    id INTEGER PRIMARY KEY,
                    title TEXT NOT NULL,
                    source TEXT,
                    date TEXT,
                    doc_type TEXT,
                    file_name TEXT,
                    content TEXT NOT NULL,
                    length INTEGER DEFAULT 0,
                    indexed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                );
            """)
            conn.execute("CREATE INDEX IF NOT EXISTS idx_doc_type ON documents(doc_type);")
            conn.execute("CREATE INDEX IF NOT EXISTS idx_doc_date ON documents(date);")
            conn.commit()

    def insert_document(self, doc_id: int, title: str, source: str, date: str,
                        doc_type: str, file_name: str, content: str, length: int = 0) -> bool:
        with self._get_connection() as conn:
            conn.execute("""
                INSERT OR REPLACE INTO documents 
                (id, title, source, date, doc_type, file_name, content, length, indexed_at)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (doc_id, title, source, date, doc_type, file_name, content, length, datetime.datetime.utcnow().isoformat()))
            conn.commit()
            return True

    def get_document(self, doc_id: int) -> Optional[Dict[str, Any]]:
        with self._get_connection() as conn:
            cursor = conn.execute("SELECT * FROM documents WHERE id = ?", (doc_id,))
            row = cursor.fetchone()
            if row:
                return dict(row)
            return None

    def delete_document(self, doc_id: int) -> bool:
        with self._get_connection() as conn:
            cursor = conn.execute("DELETE FROM documents WHERE id = ?", (doc_id,))
            conn.commit()
            return cursor.rowcount > 0

    def get_max_doc_id(self) -> int:
        with self._get_connection() as conn:
            cursor = conn.execute("SELECT COALESCE(MAX(id), 0) FROM documents")
            row = cursor.fetchone()
            return row[0] if row else 0

    def list_documents(self, doc_type: Optional[str] = None, 
                       date_start: Optional[str] = None, 
                       date_end: Optional[str] = None, 
                       limit: int = 50, offset: int = 0) -> List[Dict[str, Any]]:
        query = "SELECT id, title, source, date, doc_type, file_name, length, indexed_at FROM documents WHERE 1=1"
        params = []
        if doc_type:
            query += " AND doc_type = ?"
            params.append(doc_type)
        if date_start:
            query += " AND date >= ?"
            params.append(date_start)
        if date_end:
            query += " AND date <= ?"
            params.append(date_end)

        query += " ORDER BY id DESC LIMIT ? OFFSET ?"
        params.extend([limit, offset])

        with self._get_connection() as conn:
            cursor = conn.execute(query, params)
            return [dict(row) for row in cursor.fetchall()]

    def count_documents(self, doc_type: Optional[str] = None) -> int:
        query = "SELECT COUNT(*) FROM documents WHERE 1=1"
        params = []
        if doc_type:
            query += " AND doc_type = ?"
            params.append(doc_type)
        with self._get_connection() as conn:
            cursor = conn.execute(query, params)
            row = cursor.fetchone()
            return row[0] if row else 0

    def get_doc_types(self) -> List[str]:
        with self._get_connection() as conn:
            cursor = conn.execute("SELECT DISTINCT doc_type FROM documents WHERE doc_type IS NOT NULL AND doc_type != '' ORDER BY doc_type")
            return [row[0] for row in cursor.fetchall()]
