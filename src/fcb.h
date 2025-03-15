/* fcb.h
 * Github: https:\\github.com\tommojphillips
 */

#ifndef FCB_H
#define FCB_H

#define MAX_FILE_DESCRIPTORS 250

#define RECORD_SIZE 128
#define EXTENT_SIZE 16384
#define SECTOR_SIZE 524288

#define FILE_DESCRIPTOR_STATUS_FREE      0x00
#define FILE_DESCRIPTOR_STATUS_ALLOCATED 0x01
#define FILE_DESCRIPTOR_STATUS_OPENED    0x02

typedef struct {
	uint8_t dr;       // drive, 0 for default, 1-16 for A-P
	uint8_t fn[0x08]; // file name
	uint8_t tn[0x03]; // file ext
	uint8_t ex;       // current extent    (ptr % 524288) / EXTENT_SIZE
	uint8_t s1;
	uint8_t s2;       // extent high byte  (ptr % 524288) 
	uint8_t rc;       // record count
	uint8_t al[0x10];
	uint8_t cr;       // current record    (ptr % EXTENT_SIZE) / RECORD_SIZE
	uint8_t rn[0x03]; // Random access record number
} FILE_CONTROL_BLOCK;

typedef struct {
	uint8_t status;
	char file_name[15];
	uint32_t file_len;
	FILE* file;
} FILE_DESCRIPTOR;

int fcb_is_valid_fn(char* filename);
void fcb_get_fn(FILE_CONTROL_BLOCK* fcb, char* output);

void fcb_fill_info(FILE_CONTROL_BLOCK* fcb, const char* file_name, uint32_t size);

void fcb_get_seq_offset(FILE_CONTROL_BLOCK* fcb, uint32_t* offset);
void fcb_set_seq_offset(FILE_CONTROL_BLOCK* fcb, uint32_t offset);

void fcb_get_rand_offset(FILE_CONTROL_BLOCK* fcb, uint32_t* offset);
void fcb_set_rand_offset(FILE_CONTROL_BLOCK* fcb, uint32_t offset);

int fcb_alloc_file_descriptor(FILE_DESCRIPTOR* file_descriptors, const char* fn, FILE_DESCRIPTOR** file_descriptor);
int fcb_find_file_descriptor(FILE_DESCRIPTOR* file_descriptors, const char* fn, FILE_DESCRIPTOR** file_descriptor);
int fcb_open_file_descriptor(FILE_DESCRIPTOR* file_descriptors, const char* fn, const char* mode, FILE_DESCRIPTOR** file_descriptor);

int fcb_free_file_descriptor(FILE_DESCRIPTOR* file_descriptor);
int fcb_close_file_descriptor(FILE_DESCRIPTOR* file_descriptor);

#endif
