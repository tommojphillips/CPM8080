/* file.h
 * Github: https:\\github.com\tommojphillips
 */

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdint.h>

int read_file_into_buffer(const char* filename, void* buff, const uint32_t buff_size, const uint32_t offset, uint32_t* file_size, const uint32_t expected_size);
int get_file_size(FILE* file, uint32_t* size);
int file_exists(const char* filename);
int delete_file(const char* filename);

#endif