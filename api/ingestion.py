import os
import re
from pathlib import Path
from typing import Dict, Any, List, Optional
from .metadata_db import MetadataDB
from .indexforge_binding import IndexForgeBinding

class DocumentIngestionPipeline:
    def __init__(self, engine: IndexForgeBinding, db: MetadataDB):
        self.engine = engine
        self.db = db

    def extract_metadata_from_text(self, content: str, filename: str = "") -> Dict[str, Any]:
        """Extract title, date, doc_type, and source from Markdown or plain text"""
        meta = {
            "title": "",
            "source": "Government Repository",
            "date": "2026-01-01",
            "doc_type": "notice",
            "file_name": filename
        }

        # Try markdown title: # Title
        h1_match = re.search(r'^#\s+(.+)$', content, re.MULTILINE)
        if h1_match:
            meta["title"] = h1_match.group(1).strip()

        # Try source: **Source:** or Source: or Agency:
        source_match = re.search(r'(?:\*\*Source:\*\*|Source:|AGENCY:|Agency:)\s*([^\n\r]+)', content, re.IGNORECASE)
        if source_match:
            meta["source"] = source_match.group(1).strip()

        # Try date: **Date:** or Date:
        date_match = re.search(r'(?:\*\*Date:\*\*|Date:|DATE:)\s*([0-9]{4}-[0-9]{2}-[0-9]{2})', content, re.IGNORECASE)
        if date_match:
            meta["date"] = date_match.group(1).strip()

        # Try doc_type: **Document Type:** or Document Type:
        type_match = re.search(r'(?:\*\*Document Type:\*\*|Document Type:|DOCUMENT TYPE:)\s*([a-zA-Z_\-]+)', content, re.IGNORECASE)
        if type_match:
            raw_type = type_match.group(1).strip().lower()
            if raw_type in ("tender", "policy", "regulation", "notice", "tech_doc", "standard"):
                meta["doc_type"] = raw_type

        # Fallback title from filename or first line
        if not meta["title"]:
            if filename:
                meta["title"] = Path(filename).stem.replace("_", " ").replace("-", " ").title()
            else:
                first_line = content.strip().split("\n")[0]
                meta["title"] = first_line[:80].strip() or "Untitled Document"

        return meta

    def ingest_single(self, content: str, title: Optional[str] = None, source: Optional[str] = None,
                      date: Optional[str] = None, doc_type: Optional[str] = None,
                      filename: str = "") -> Dict[str, Any]:
        extracted = self.extract_metadata_from_text(content, filename)

        final_title = title if title else extracted["title"]
        final_source = source if source else extracted["source"]
        final_date = date if date else extracted["date"]
        final_type = doc_type if doc_type else extracted["doc_type"]

        doc_id = self.db.get_max_doc_id() + 1

        # Store in SQLite metadata DB
        self.db.insert_document(
            doc_id=doc_id,
            title=final_title,
            source=final_source,
            date=final_date,
            doc_type=final_type,
            file_name=filename,
            content=content,
            length=len(content.split())
        )

        # Index in C++ Inverted Index Core
        ok = self.engine.index_document(
            doc_id=doc_id,
            title=final_title,
            source=final_source,
            date=final_date,
            doc_type=final_type,
            content=content
        )

        return {
            "success": ok,
            "doc_id": doc_id,
            "title": final_title,
            "source": final_source,
            "date": final_date,
            "doc_type": final_type,
            "file_name": filename
        }

    def ingest_directory(self, dir_path: str) -> List[Dict[str, Any]]:
        results = []
        p = Path(dir_path)
        if not p.exists() or not p.is_dir():
            return results

        for file_path in sorted(p.iterdir()):
            if file_path.suffix.lower() in (".txt", ".md", ".json", ".log"):
                try:
                    with open(file_path, "r", encoding="utf-8", errors="replace") as f:
                        content = f.read()
                    res = self.ingest_single(content=content, filename=file_path.name)
                    results.append(res)
                except Exception as e:
                    results.append({
                        "success": False,
                        "file_name": file_path.name,
                        "error": str(e)
                    })
        return results
