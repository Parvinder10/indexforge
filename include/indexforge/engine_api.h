#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(INDEXFORGE_STATIC)
  #define INDEXFORGE_API
#elif defined(_WIN32)
  #ifdef INDEXFORGE_EXPORTS
    #define INDEXFORGE_API __declspec(dllexport)
  #else
    #define INDEXFORGE_API __declspec(dllimport)
  #endif
#else
  #define INDEXFORGE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* indexforge_handle;

// Create new engine instance
INDEXFORGE_API indexforge_handle indexforge_create();

// Destroy engine instance
INDEXFORGE_API void indexforge_destroy(indexforge_handle handle);

// Ingest / index a document
INDEXFORGE_API int indexforge_index_document(
    indexforge_handle handle,
    uint32_t doc_id,
    const char* title,
    const char* source,
    const char* date,
    const char* doc_type,
    const char* content);

// Delete document by id
INDEXFORGE_API int indexforge_delete_document(
    indexforge_handle handle,
    uint32_t doc_id);

// Update document
INDEXFORGE_API int indexforge_update_document(
    indexforge_handle handle,
    uint32_t doc_id,
    const char* title,
    const char* source,
    const char* date,
    const char* doc_type,
    const char* content);

// Search using Inverted Index + Ranking (0 = BM25, 1 = TF_IDF)
// Returns allocated JSON string (must be freed with indexforge_free_string)
INDEXFORGE_API const char* indexforge_search(
    indexforge_handle handle,
    const char* query,
    int ranking_algo,
    double k1,
    double b,
    uint32_t limit,
    uint32_t offset);

// Baseline Linear Scan search across raw documents for comparative benchmark
// Returns allocated JSON string (must be freed with indexforge_free_string)
INDEXFORGE_API const char* indexforge_linear_search(
    indexforge_handle handle,
    const char* query,
    uint32_t limit);

// Get index statistics (JSON string)
INDEXFORGE_API const char* indexforge_get_stats(indexforge_handle handle);

// Save index to binary file
INDEXFORGE_API int indexforge_save_index(
    indexforge_handle handle,
    const char* filepath);

// Load index from binary file
INDEXFORGE_API int indexforge_load_index(
    indexforge_handle handle,
    const char* filepath);

// Verify index file integrity (returns JSON result)
INDEXFORGE_API const char* indexforge_verify_integrity(const char* filepath);

// Clear query cache
INDEXFORGE_API void indexforge_clear_cache(indexforge_handle handle);

// Compact index (remove deleted documents from postings)
INDEXFORGE_API void indexforge_compact(indexforge_handle handle);

// Free heap string returned by C-API
INDEXFORGE_API void indexforge_free_string(char* str);

#ifdef __cplusplus
}
#endif
