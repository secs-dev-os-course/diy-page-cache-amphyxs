#pragma once
#include <vector>
#include "cached_file.h"

class Page final
{
public:
    Page(int id, CachedFile *file);

    ~Page();

    void mark_as_mru();

    const std::vector<char> &get_data();

    void set_data(std::vector<char> data);

    void set_data(const char *data, size_t offset, size_t size);

    void upload_to_disk();

private:
    int index;
    CachedFile *file;
    std::vector<char> data;

    bool check_pages_limit_exceeded();

    void free_mru_page();
};