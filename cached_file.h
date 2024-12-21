#pragma once
#include <unordered_map>
#include <Windows.h>
#include "page.h"
#include <memory>
#include <chrono>

class Page;

class CachedFile
{
public:
    std::unordered_map<int, std::unique_ptr<Page>> pages;
    int file_id;           // Unique ID for the file in the cache
    HANDLE windows_handle; // Windows file handle for operations
    int position;
    int file_size;
    std::chrono::time_point<std::chrono::steady_clock> last_modification_in_cache;

    CachedFile(const char *file_path);

    ~CachedFile();

    void read_page_from_disk(size_t page_index);

    static CachedFile *get_file_by_id(int id);

    void load_or_initialize_page(size_t page_index);

    void update_last_modification_time();

    void clear_cached_pages();

    void upload_cache_on_disk();

protected:
    LPCWSTR cchararr_to_lpcwstr(const char *cchararr);
};