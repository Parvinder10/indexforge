import sys
import os
from pathlib import Path

# Add project root to sys.path
root_dir = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root_dir))

from api.indexforge_binding import IndexForgeBinding
from api.metadata_db import MetadataDB
from api.ingestion import DocumentIngestionPipeline

def test_full_pipeline():
    print("Testing IndexForge Python & C++ Pipeline...")

    # 1. Initialize Binding
    engine = IndexForgeBinding()
    print("Engine initialized successfully.")

    # 2. Initialize DB
    test_db_path = str(root_dir / "data" / "test_meta.db")
    if os.path.exists(test_db_path):
        os.remove(test_db_path)
    db = MetadataDB(test_db_path)
    print("Metadata DB initialized successfully.")

    # 3. Ingest documents
    pipeline = DocumentIngestionPipeline(engine, db)
    doc1 = pipeline.ingest_single(
        content="Public tender notice for federal cloud cybersecurity modernization and zero trust architecture.",
        title="Federal Tender Notice",
        source="General Services Administration",
        date="2026-03-01",
        doc_type="tender"
    )
    assert doc1["success"], "Failed to ingest doc1"
    print(f"Ingested doc 1: {doc1['title']}")

    doc2 = pipeline.ingest_single(
        content="Compliance policy guidelines for cryptographic encryption and mandatory incident reporting.",
        title="Cybersecurity Compliance Policy",
        source="CISA",
        date="2026-04-15",
        doc_type="policy"
    )
    assert doc2["success"], "Failed to ingest doc2"
    print(f"Ingested doc 2: {doc2['title']}")

    # 4. Search BM25
    res = engine.search("tender AND cybersecurity")
    print("Search 'tender AND cybersecurity' results:", res)
    assert res["total_hits"] >= 1, "Expected at least 1 hit for 'tender AND cybersecurity'"

    # 5. Search Phrase
    phrase_res = engine.search('"zero trust"')
    print('Search \'"zero trust"\' results:', phrase_res)
    assert phrase_res["total_hits"] >= 1, "Expected hit for '\"zero trust\"'"

    # 6. Check Cache
    cached_res = engine.search("tender AND cybersecurity")
    assert cached_res.get("from_cache") == True, "Expected query to be served from cache"
    print("Cache hit verified!")

    # 7. Check Stats
    stats = engine.get_stats()
    print("Engine Stats:", stats)
    assert stats["active_documents"] == 2, "Expected 2 active documents"

    # Cleanup test DB if possible
    try:
        if os.path.exists(test_db_path):
            os.remove(test_db_path)
    except Exception:
        pass

    print("\nAll Python/C++ pipeline integration tests PASSED!")

if __name__ == "__main__":
    test_full_pipeline()
