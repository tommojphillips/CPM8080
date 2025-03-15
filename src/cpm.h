/* cpm.h
 * Github: https:\\github.com\tommojphillips
 */

#include <stdint.h>

#include "i8080.h"
#include "fcb.h"

#ifndef CPM_H
#define CPM_H

#define MAX_DISK_DESCRIPTORS 16

typedef struct {
	char file_name[16];
	uint32_t file_len;
	FILE* file;
	uint8_t status;
} DISK_DESCRIPTOR;

typedef struct {
	I8080 cpu;
	char str_delimiter;
	uint8_t selected_drive;
	uint8_t user_num;
	uint16_t dma_address;
	char fn_buffer[16];
	FILE_CONTROL_BLOCK* fcb1;
	FILE_CONTROL_BLOCK* fcb2;
	uint8_t* memory;
	FILE_DESCRIPTOR* files;
	DISK_DESCRIPTOR* disks;
} CPM;

#ifdef __cplusplus
extern "C" {
#endif

extern CPM cpm;
int cpm_init();
void cpm_destroy();
void cpm_reset();
int cpm_update();

void cpm_load_com(const char* arg);
void cpm_load_hex(const char* arg);
void cpm_load_params(const char* arg);

#ifdef __cplusplus
};
#endif
#endif
