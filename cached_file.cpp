#include "cached_file.h"
#include "util.cpp"
#include "consts.cpp"

#include <stdexcept>

std::unordered_map<int, CachedFile *> cache;
int next_file_id = 1;

CachedFile::CachedFile(const char *file_path)
{
    this->windows_handle = CreateFile(cchararr_to_lpcwstr(file_path), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                      FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (this->windows_handle == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("Error opening the file");
    }

    this->file_id = next_file_id++;
    this->position = 0;
    this->file_size = GetFileSize(this->windows_handle, NULL);
    update_last_modification_time();

    cache[this->file_id] = this;
}

CachedFile::~CachedFile()
{
    clear_cached_pages();
    cache.erase(this->file_id);
    CloseHandle(this->windows_handle);
    log("destroy file id: ", this->file_id);
}

void CachedFile::read_page_from_disk(int page_index)
{
    log("(x_x ) cache miss, page id: ", page_index);

    std::vector<char> page_data(PAGE_SIZE);
    DWORD bytes_read;

    SetFilePointer(this->windows_handle, page_index * PAGE_SIZE, NULL, FILE_BEGIN);

    if (!ReadFile(this->windows_handle, page_data.data(), PAGE_SIZE, &bytes_read, NULL))
    {
        throw std::runtime_error("Failed to read file: " + get_windows_error_message());
    }

    Page *page = new Page(page_index, this);
    page->set_data(std::move(page_data));
    this->pages[page_index] = page;
}

LPCWSTR CachedFile::cchararr_to_lpcwstr(const char *cchararr)
{
    int length = MultiByteToWideChar(CP_UTF8, 0, cchararr, -1, NULL, 0);
    wchar_t *result = new wchar_t[length];
    MultiByteToWideChar(CP_UTF8, 0, cchararr, -1, result, length);
    return result;
}

CachedFile *CachedFile::get_file_by_id(int id)
{
    auto it = cache.find(id);
    if (it == cache.end())
    {
        throw std::runtime_error("File was not opened");
    }

    return it->second;
}

void CachedFile::load_or_initialize_page(int page_index)
{
    int start_byte = page_index * PAGE_SIZE;

    if (start_byte > this->file_size)
    {
        Page *new_page = new Page(page_index, this);
        this->pages[page_index] = new_page;
        this->file_size += PAGE_SIZE;
    }
    else
    {
        read_page_from_disk(page_index);
    }
}

void CachedFile::update_last_modification_time()
{
    GetSystemTimeAsFileTime(&this->last_modification_in_cache);
}

void CachedFile::clear_cached_pages()
{
    while (!this->pages.empty())
    {
        delete this->pages.begin()->second;
    }
}

void CachedFile::upload_cache_on_disk()
{
    for (auto &page_entry : this->pages)
    {
        page_entry.second->upload_to_disk();
    }

    // Update the last modification time after uploading the cache
    update_last_modification_time();
}
