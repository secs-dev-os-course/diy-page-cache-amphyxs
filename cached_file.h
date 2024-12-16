#pragma once
#include <unordered_map>
#include <Windows.h>
#include "page.h"

class Page;

class CachedFile
{
public:
    std::unordered_map<int, Page *> pages;
    int file_id;           // Unique ID for the file in the cache
    HANDLE windows_handle; // Windows file handle for operations
    int position;
    int file_size;
    FILETIME last_modification_in_cache;

    CachedFile(const char *file_path);

    ~CachedFile();

    void read_page_from_disk(int page_index);

    static CachedFile *get_file_by_id(int id);

    void load_or_initialize_page(int page_index);

    void update_last_modification_time();

    void clear_cached_pages();

    void upload_cache_on_disk();

protected:
    LPCWSTR cchararr_to_lpcwstr(const char *cchararr);
};