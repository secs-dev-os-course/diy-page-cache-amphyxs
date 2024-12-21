#include "page.h"
#include "util.cpp"
#include "consts.cpp"

Page *mru_page = nullptr;
int pages_count = 0;

Page::Page(int id, CachedFile *file)
{
    this->index = id;
    this->file = file;

    if (check_pages_limit_exceeded())
    {
        free_mru_page();
    }

    pages_count++;
    log("new page, total: ", pages_count, "/", MAX_PAGES_COUNT);
}

Page::~Page()
{
    data.clear();
    file->pages.erase(this->index);
    pages_count--;
    log("free page index: ", this->index, ", total: ", pages_count, "/", MAX_PAGES_COUNT);
}

void Page::mark_as_mru()
{
    mru_page = this;
    log("MRU page index: ", index);
}

const std::vector<char> &Page::get_data()
{
    mark_as_mru();
    return this->data;
}

void Page::set_data(std::vector<char> data)
{
    mark_as_mru();
    this->data = data;
}

void Page::set_data(const char *data, size_t offset, size_t size)
{
    mark_as_mru();
    memcpy(this->data.data() + offset, data, size);
}

bool Page::check_pages_limit_exceeded()
{
    return pages_count >= MAX_PAGES_COUNT;
}

void Page::free_mru_page()
{
    if (mru_page == nullptr)
    {
        log("wataheeeeell!?!?!?!? mru_page is nullptr");
        return;
    }

    mru_page->upload_to_disk();
    delete mru_page;
}

void Page::upload_to_disk()
{
    // Set the file pointer to the correct location
    DWORD bytes_written;
    int start_byte = this->index * PAGE_SIZE;
    SetFilePointer(this->file->windows_handle, start_byte, NULL, FILE_BEGIN);

    if (!WriteFile(this->file->windows_handle, this->get_data().data(), PAGE_SIZE, &bytes_written, NULL))
    {
        throw std::runtime_error("Failed to write page " + std::to_string(this->index) + " to disk: " + get_windows_error_message());
    }

    log("uploaded page ", this->index, " to disk");
}
