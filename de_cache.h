#pragma once

#ifdef DECACHE_EXPORTS
#define DECACHE_API __declspec(dllexport)
#else
#define DECACHE_API __declspec(dllimport)
#endif

extern "C" DECACHE_API int de_cache_open(const char *path);

extern "C" DECACHE_API size_t de_cache_read(int file_id, char *buffer, size_t size);

extern "C" DECACHE_API size_t de_cache_write(int file_id, const char *buffer, size_t size);

extern "C" DECACHE_API void de_cache_set_file_position(int file_id, int position);

extern "C" DECACHE_API void de_cache_close(int file_id);

extern "C" DECACHE_API void de_cache_sync(int file_id);
