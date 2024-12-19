#include "pch.h"
#include <Windows.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include "de_cache.h"
#include "util.cpp"
#include "page.cpp"
#include "cached_file.cpp"

int de_cache_open(const char *path)
{
    CachedFile *cached_file = new CachedFile(path);
    log("create empty cached file, id: ", cached_file->file_id, ", path: ", path);

    return cached_file->file_id;
}

size_t de_cache_read(int file_id, char *buffer, size_t size)
{

    CachedFile *cached_file = CachedFile::get_file_by_id(file_id);
    size_t bytes_read_total = 0;
    size_t offset = cached_file->position;

    if (offset >= cached_file->file_size)
    {
        return 0;
    }

    while (bytes_read_total < size)
    {
        int page_index = offset / PAGE_SIZE;
        int page_offset = offset % PAGE_SIZE;
        int bytes_to_read = _min(PAGE_SIZE - page_offset, size - bytes_read_total);

        log("reading ", bytes_read_total, "/", size, " bytes");

        auto page_it = cached_file->pages.find(page_index);
        if (page_it == cached_file->pages.end())
        {
            // Page not in cache
            cached_file->read_page_from_disk(page_index);
            page_it = cached_file->pages.find(page_index);
        }

        // Copy data from cache to buffer
        log("copying bytes [", page_offset, "-", page_offset + bytes_to_read, "] from page index: ", page_index);
        memcpy(buffer + bytes_read_total, page_it->second->get_data().data() + page_offset, bytes_to_read);
        bytes_read_total += bytes_to_read;
        offset += bytes_to_read;

        if (offset >= cached_file->file_size)
        {
            return bytes_read_total;
        }
    }

    return bytes_read_total;
}

size_t de_cache_write(int file_id, const char *data, size_t size)
{
    CachedFile *cached_file = CachedFile::get_file_by_id(file_id);
    size_t bytes_written_total = 0;
    size_t offset = cached_file->position;

    while (bytes_written_total < size)
    {
        size_t page_index = offset / PAGE_SIZE;
        size_t page_offset = offset % PAGE_SIZE;
        size_t bytes_to_write = _min(PAGE_SIZE - page_offset, size - bytes_written_total);

        log("writing ", bytes_written_total, "/", size, " bytes");

        // Check if the page is in cache
        auto page_it = cached_file->pages.find(page_index);
        if (page_it == cached_file->pages.end())
        {
            // Page not in cache, load from disk or initialize a new page if beyond current file size
            cached_file->load_or_initialize_page(page_index);
            page_it = cached_file->pages.find(page_index);
        }

        // Write data to the cache page
        log("writing bytes [", page_offset, "-", page_offset + bytes_to_write, "] to page index: ", page_index);
        memcpy(page_it->second->get_data().data() + page_offset, data + bytes_written_total, bytes_to_write);
        cached_file->update_last_modification_time();

        bytes_written_total += bytes_to_write;
        offset += bytes_to_write;
    }

    return bytes_written_total;
}

void de_cache_set_file_position(int file_id, int position)
{
    CachedFile *file = CachedFile::get_file_by_id(file_id);
    log("Setting position: ", position, "/", file->file_size);
    if (position >= file->file_size)
    {
        log("Trying to set position over the file end");
    }

    file->position = position;
}

void de_cache_close(int file_id)
{
    delete CachedFile::get_file_by_id(file_id);
}

void de_cache_sync(int file_id)
{
    CachedFile *file = CachedFile::get_file_by_id(file_id);
    FILETIME external_last_modification;
    if (!GetFileTime(file->windows_handle, NULL, NULL, &external_last_modification))
    {
        throw std::runtime_error("Error getting file time");
    }

    TimeComparison in_cache_than_external = compare_times(file->last_modification_in_cache, external_last_modification);

    if (in_cache_than_external == EARLIER)
    {
        log("(T_T ) file with id ", file_id, " is outdated in cache, clearing all cached pages");
        file->clear_cached_pages();
        file->file_size = GetFileSize(
            file->windows_handle,
            NULL // High part of number for big numbers
        );
    }
    else if (in_cache_than_external == LATER)
    {
        log("(`^` ) file with id ", file_id, " is newer than on disk, uploading all cached pages on the disk");
        file->upload_cache_on_disk();
    }
    else
    {
        log("(^v^ ) file with id ", file_id, " is up to date to the disk version");
    }
}
