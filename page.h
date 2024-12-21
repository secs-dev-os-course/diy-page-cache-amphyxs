#pragma once
#include <vector>
#include "cached_file.h"

class Page final
{
public:
    Page(int id, CachedFile *file);

    ~Page();

    void mark_as_mru();

    std::vector<char> get_data();

    void set_data(std::vector<char> data);

    void upload_to_disk();

protected:
    std::vector<char> data;

    bool check_pages_limit_exceeded();

    void free_mru_page();

private:
    int index;
    CachedFile *file;
};