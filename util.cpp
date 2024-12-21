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

int divide_ceiled(int a, int b)
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

TimeComparison compare_times(std::chrono::time_point<std::chrono::steady_clock> &tp1, const FILETIME &ft2)
{
    // Convert FILETIME to std::chrono::steady_clock::time_point for comparison
    ULARGE_INTEGER uli;
    uli.LowPart = ft2.dwLowDateTime;
    uli.HighPart = ft2.dwHighDateTime;
    auto tp2 = std::chrono::time_point<std::chrono::steady_clock>(std::chrono::nanoseconds(uli.QuadPart * 100));

    if (tp1 < tp2)
    {
        return EARLIER;
    }
    else if (tp1 == tp2)
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
