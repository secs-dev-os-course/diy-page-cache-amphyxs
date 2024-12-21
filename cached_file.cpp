#include "cached_file.h"
#include "util.cpp"
#include "consts.cpp"

#include <stdexcept>
#include <memory>
#include <chrono>

std::unordered_map<int, CachedFile *> cache;
int next_file_id = 1;

CachedFile::CachedFile(const char *file_path)
{
    this->windows_handle = CreateFile(
        cchararr_to_lpcwstr(file_path),                     // Path to file, must be a type of LPCWSTR
        GENERIC_ALL,                                        // Desired access to file, here we want to read and write into file
        FILE_SHARE_READ | FILE_SHARE_WRITE,                 // File sharing mode between processes
        NULL,                                               // Security attributes, NULL tells to use default security attributes
        OPEN_EXISTING,                                      // Creating disposition, here will throw error if the file doesn't exist
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, // Flags and attributes, here we set some flags for OS page caching disabling
        NULL                                                // Template file, we don't need it
    );

    if (this->windows_handle == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("Error opening the file");
    }

    // Obtain file information
    BY_HANDLE_FILE_INFORMATION file_info;
    if (GetFileInformationByHandle(this->windows_handle, &file_info))
    {
        // Combine volume serial number and file index to form a unique file ID
        this->file_id = (static_cast<uint64_t>(file_info.nFileIndexHigh) << 32) | file_info.nFileIndexLow;
    }
    else
    {
        // Fallback to incrementing ID if unique file ID cannot be determined
        this->file_id = next_file_id++;
    }

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

void CachedFile::read_page_from_disk(size_t page_index)
{
    log("(x_x ) cache miss, page id: ", page_index);

    std::vector<char> page_data(PAGE_SIZE);
    DWORD bytes_read;

    if (SetFilePointer(
            this->windows_handle,   // Handle of target file
            page_index * PAGE_SIZE, // Pointer position
            NULL,                   // High part of number pointer position, used for long numbers
            FILE_BEGIN              // Move method, we want to calculate pointer position from file start
            ) == INVALID_SET_FILE_POINTER)
    {
        throw std::runtime_error("Failed to set file pointer while reading file: " + get_windows_error_message());
    }

    if (!ReadFile(
            this->windows_handle, // File to read handle
            page_data.data(),     // Where to read
            PAGE_SIZE,            // How much to read
            &bytes_read,          // Total bytes read for checking
            NULL                  // Pointer to OVERLAPPED struct, we don't need it
            ))
    {
        throw std::runtime_error("Failed to read file: " + get_windows_error_message());
    }

    std::unique_ptr<Page> page = std::make_unique<Page>(page_index, this);
    page->set_data(std::move(page_data));
    this->pages[page_index] = std::move(page);
}

LPCWSTR CachedFile::cchararr_to_lpcwstr(const char *cchararr)
{
    int length = MultiByteToWideChar(
        CP_UTF8,  // Encoding of char
        0,        // Flags
        cchararr, // String to convert
        -1,       // Marks the string as null-terminated
        NULL,     // Pointer to a buffer that receives the converted string
        0         // 0 tells the function to return a desired buffer size
    );
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

void CachedFile::load_or_initialize_page(size_t page_index)
{
    int start_byte = page_index * PAGE_SIZE;

    if (start_byte > this->file_size)
    {
        std::unique_ptr<Page> new_page = std::make_unique<Page>(page_index, this);
        this->pages[page_index] = std::move(new_page);
        this->file_size += PAGE_SIZE;
    }
    else
    {
        read_page_from_disk(page_index);
    }
}

void CachedFile::update_last_modification_time()
{
    this->last_modification_in_cache = std::chrono::steady_clock::now();
}

void CachedFile::clear_cached_pages()
{
    while (!this->pages.empty())
    {
        this->pages.begin()->second.reset();
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
