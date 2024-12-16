#pragma once
#include "consts.cpp"

template <typename... Args>
void log(Args... args)
{
    if (!ENABLE_DEBUG_LOG)
    {
        return;
    }

    std::ostringstream stream;
    // Use the initializer list trick to expand the variadic template arguments
    // and append them to the stringstream
    (stream << ... << args);

    std::cout << "[de_cache]: " << stream.str() << std::endl;
}

int ceil(int a, int b)
{
    return (a + b - 1) / b;
}

int _min(int a, int b)
{
    return (((a) < (b)) ? (a) : (b));
}

enum TimeComparison
{
    EARLIER,
    SAME,
    LATER
};

TimeComparison compare_times(const FILETIME &ft1, const FILETIME &ft2)
{
    // Convert FILETIME to LARGE_INTEGER for comparison
    LARGE_INTEGER li1, li2;
    li1.LowPart = ft1.dwLowDateTime;
    li1.HighPart = ft1.dwHighDateTime;
    li2.LowPart = ft2.dwLowDateTime;
    li2.HighPart = ft2.dwHighDateTime;

    if (li1.QuadPart < li2.QuadPart)
    {
        return EARLIER;
    }
    else if (li1.QuadPart == li2.QuadPart)
    {
        return SAME;
    }
    else
    {
        return LATER;
    }
}

std::string get_windows_error_message()
{
    DWORD error_code = GetLastError();
    LPSTR message_buffer = nullptr;

    // Format the error message into a string
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error_code, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPSTR)&message_buffer, 0, NULL);

    std::string error_message(message_buffer, size);

    // Free the buffer allocated by FormatMessage
    LocalFree(message_buffer);

    return error_message;
}
