import os
import sys
import ctypes
import json
from pathlib import Path
from typing import Dict, Any, Optional

class IndexForgeBinding:
    def __init__(self, dll_path: Optional[str] = None):
        if dll_path is None:
            # Locate build/indexforge.dll
            base_dir = Path(__file__).resolve().parent.parent
            candidates = [
                base_dir / "build" / "indexforge.dll",
                base_dir / "build" / "Release" / "indexforge.dll",
                base_dir / "indexforge.dll"
            ]
            for c in candidates:
                if c.exists():
                    dll_path = str(c)
                    if hasattr(os, "add_dll_directory"):
                        try:
                            os.add_dll_directory(str(c.parent))
                        except Exception:
                            pass
                    break

        if not dll_path or not os.path.exists(dll_path):
            raise FileNotFoundError(f"Could not locate indexforge shared library at {dll_path}")

        self.lib = ctypes.CDLL(dll_path)

        # Set up argument and return types
        self.lib.indexforge_create.restype = ctypes.c_void_p
        self.lib.indexforge_create.argtypes = []

        self.lib.indexforge_destroy.restype = None
        self.lib.indexforge_destroy.argtypes = [ctypes.c_void_p]

        self.lib.indexforge_index_document.restype = ctypes.c_int
        self.lib.indexforge_index_document.argtypes = [
            ctypes.c_void_p, ctypes.c_uint32, ctypes.c_char_p,
            ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p
        ]

        self.lib.indexforge_delete_document.restype = ctypes.c_int
        self.lib.indexforge_delete_document.argtypes = [ctypes.c_void_p, ctypes.c_uint32]

        self.lib.indexforge_update_document.restype = ctypes.c_int
        self.lib.indexforge_update_document.argtypes = [
            ctypes.c_void_p, ctypes.c_uint32, ctypes.c_char_p,
            ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p
        ]

        self.lib.indexforge_search.restype = ctypes.c_void_p
        self.lib.indexforge_search.argtypes = [
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int,
            ctypes.c_double, ctypes.c_double, ctypes.c_uint32, ctypes.c_uint32
        ]

        self.lib.indexforge_linear_search.restype = ctypes.c_void_p
        self.lib.indexforge_linear_search.argtypes = [
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint32
        ]

        self.lib.indexforge_get_stats.restype = ctypes.c_void_p
        self.lib.indexforge_get_stats.argtypes = [ctypes.c_void_p]

        self.lib.indexforge_save_index.restype = ctypes.c_int
        self.lib.indexforge_save_index.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        self.lib.indexforge_load_index.restype = ctypes.c_int
        self.lib.indexforge_load_index.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        self.lib.indexforge_verify_integrity.restype = ctypes.c_void_p
        self.lib.indexforge_verify_integrity.argtypes = [ctypes.c_char_p]

        self.lib.indexforge_clear_cache.restype = None
        self.lib.indexforge_clear_cache.argtypes = [ctypes.c_void_p]

        self.lib.indexforge_compact.restype = None
        self.lib.indexforge_compact.argtypes = [ctypes.c_void_p]

        self.lib.indexforge_free_string.restype = None
        self.lib.indexforge_free_string.argtypes = [ctypes.c_void_p]

        self.handle = self.lib.indexforge_create()
        if not self.handle:
            raise RuntimeError("Failed to create IndexForge engine instance")

    def __del__(self):
        if hasattr(self, 'handle') and self.handle and hasattr(self, 'lib') and self.lib:
            self.lib.indexforge_destroy(self.handle)
            self.handle = None

    def index_document(self, doc_id: int, title: str, source: str, date: str, doc_type: str, content: str) -> bool:
        rc = self.lib.indexforge_index_document(
            self.handle,
            ctypes.c_uint32(doc_id),
            title.encode('utf-8', errors='replace'),
            source.encode('utf-8', errors='replace'),
            date.encode('utf-8', errors='replace'),
            doc_type.encode('utf-8', errors='replace'),
            content.encode('utf-8', errors='replace')
        )
        return rc == 0

    def delete_document(self, doc_id: int) -> bool:
        rc = self.lib.indexforge_delete_document(self.handle, ctypes.c_uint32(doc_id))
        return rc == 0

    def update_document(self, doc_id: int, title: str, source: str, date: str, doc_type: str, content: str) -> bool:
        rc = self.lib.indexforge_update_document(
            self.handle,
            ctypes.c_uint32(doc_id),
            title.encode('utf-8', errors='replace'),
            source.encode('utf-8', errors='replace'),
            date.encode('utf-8', errors='replace'),
            doc_type.encode('utf-8', errors='replace'),
            content.encode('utf-8', errors='replace')
        )
        return rc == 0

    def search(self, query: str, algo: str = "bm25", k1: float = 1.2, b: float = 0.75, limit: int = 20, offset: int = 0) -> Dict[str, Any]:
        algo_code = 1 if algo.lower() in ("tfidf", "tf_idf") else 0
        raw_res = self.lib.indexforge_search(
            self.handle,
            query.encode('utf-8', errors='replace'),
            ctypes.c_int(algo_code),
            ctypes.c_double(k1),
            ctypes.c_double(b),
            ctypes.c_uint32(limit),
            ctypes.c_uint32(offset)
        )
        if not raw_res:
            return {"hits": [], "total_hits": 0, "latency_ms": 0.0}
        
        json_str = ctypes.string_at(raw_res).decode('utf-8', errors='replace')
        self.lib.indexforge_free_string(raw_res)
        return json.loads(json_str)

    def linear_search(self, query: str, limit: int = 20) -> Dict[str, Any]:
        raw_res = self.lib.indexforge_linear_search(
            self.handle,
            query.encode('utf-8', errors='replace'),
            ctypes.c_uint32(limit)
        )
        if not raw_res:
            return {"hits": [], "total_hits": 0, "latency_ms": 0.0}

        json_str = ctypes.string_at(raw_res).decode('utf-8', errors='replace')
        self.lib.indexforge_free_string(raw_res)
        return json.loads(json_str)

    def get_stats(self) -> Dict[str, Any]:
        raw_res = self.lib.indexforge_get_stats(self.handle)
        if not raw_res:
            return {}
        json_str = ctypes.string_at(raw_res).decode('utf-8', errors='replace')
        self.lib.indexforge_free_string(raw_res)
        return json.loads(json_str)

    def save_index(self, filepath: str) -> bool:
        rc = self.lib.indexforge_save_index(self.handle, filepath.encode('utf-8'))
        return rc == 0

    def load_index(self, filepath: str) -> bool:
        rc = self.lib.indexforge_load_index(self.handle, filepath.encode('utf-8'))
        return rc == 0

    def verify_integrity(self, filepath: str) -> Dict[str, Any]:
        raw_res = self.lib.indexforge_verify_integrity(filepath.encode('utf-8'))
        if not raw_res:
            return {"valid": False, "error": "unknown error"}
        json_str = ctypes.string_at(raw_res).decode('utf-8', errors='replace')
        self.lib.indexforge_free_string(raw_res)
        return json.loads(json_str)

    def clear_cache(self):
        self.lib.indexforge_clear_cache(self.handle)

    def compact(self):
        self.lib.indexforge_compact(self.handle)
