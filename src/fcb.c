/* fcb.c
 * Github: https:\\github.com\tommojphillips
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fcb.h"
#include "file.h"

int fcb_is_valid_fn(char* filename) {
	char* ptr1;
	if ((ptr1 = strchr(filename, '.')) != NULL) {
		if ((ptr1 - filename) <= 8) {
			if (strlen(ptr1 + 1) <= 3) {
				return 0; // valid
			}
		}
	}
	else {
		if (strlen(filename) <= 8) {
			return 0; // valid
		}
	}
	return 1; // invalid
}

void fcb_get_fn(FILE_CONTROL_BLOCK* fcb, char* output) {
	int i = 0;
	for (int j = 0; j < 8; ++j) {
		if (fcb->fn[j] == 0x20 || fcb->fn[j] == 0) break;
		output[i++] = fcb->fn[j];
	}
	if (i > 0) {
		output[i++] = '.';
		int j = i;
		for (; j < i + 3; ++j) {
			if (fcb->tn[j - i] == 0x20 || fcb->tn[j - i] == 0) break;
			output[j] = fcb->tn[j - i];
		}
		if (i < j) {
			i = j;
		}
		else {
			--i;
		}
	}
	output[i] = '\0';
}

void fcb_fill_info(FILE_CONTROL_BLOCK* fcb, const char* file_name, uint32_t file_size) {
	fcb->rc = ((file_size % EXTENT_SIZE) / RECORD_SIZE) & 0xFF;
	fcb->dr = 0;
	
	int i = 0;
	int ext_found = 0;
	size_t len = strlen(file_name);
	for (int j = 0; j < 8; ++j) {

		if (i >= len) {
			ext_found = 0;
			fcb->fn[j] = 0x20;
			continue;
		}

		if (ext_found == 0) {
			if (file_name[i] == '.') {
				ext_found = 1;
				++i;
				fcb->fn[j] = 0x20;
				continue;
			}
			fcb->fn[j] = file_name[i++];
		}
		else {
			fcb->fn[j] = 0x20;
		}
	}
		
	for (int j = 0; j < 3; ++j) {

		if (i >= len) {
			ext_found = 0;
			fcb->tn[j] = 0x20;
			continue;
		}

		if (file_name[i] == '.') {
			ext_found = 1;
			++i;
		}

		if (ext_found == 1) {
			fcb->tn[j] = file_name[i++];
		}
		else {
			fcb->tn[j] = 0x20;
		}
	}
}

void fcb_get_seq_offset(FILE_CONTROL_BLOCK* fcb, uint32_t* offset) {
	*offset = fcb->s2 * SECTOR_SIZE + fcb->ex * EXTENT_SIZE + fcb->cr * RECORD_SIZE;
}
void fcb_set_seq_offset(FILE_CONTROL_BLOCK* fcb, uint32_t offset) {
	fcb->cr = ((offset % EXTENT_SIZE) / RECORD_SIZE) & 0xFF;
	fcb->ex = ((offset % SECTOR_SIZE) / EXTENT_SIZE) & 0xFF;
	fcb->s2 = (offset / SECTOR_SIZE) & 0xFF;
}
void fcb_get_rand_offset(FILE_CONTROL_BLOCK* fcb, uint32_t* offset) {
	*offset = fcb->rn[1] * 32768 + fcb->rn[0] * RECORD_SIZE;
}
void fcb_set_rand_offset(FILE_CONTROL_BLOCK* fcb, uint32_t offset) {
	fcb->rn[0] = (offset / RECORD_SIZE) % 256;
	uint32_t t = (offset / RECORD_SIZE) / 256;
	fcb->rn[1] = t & 0xFF;
	fcb->rn[2] = (t >> 8) & 0xFF;
}

int fcb_alloc_file_descriptor(FILE_DESCRIPTOR* file_descriptors, const char* fn, FILE_DESCRIPTOR** file_descriptor) {
	for (int i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		if ((file_descriptors[i].status & FILE_DESCRIPTOR_STATUS_ALLOCATED) == 0) {
			*file_descriptor = &file_descriptors[i];
			strcpy_s(file_descriptors[i].file_name, strlen(fn) + 1, fn);
			file_descriptors[i].status = FILE_DESCRIPTOR_STATUS_ALLOCATED;
			return i;
		}
	}
	return -1; // no more allocations left
}
int fcb_find_file_descriptor(FILE_DESCRIPTOR* file_descriptors, const char* fn, FILE_DESCRIPTOR** file_descriptor) {
	for (int i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		if (file_descriptors[i].status & FILE_DESCRIPTOR_STATUS_ALLOCATED) {
			if (strcmp(file_descriptors[i].file_name, fn) == 0) {
				*file_descriptor = &file_descriptors[i];
				return i;
			}
		}
	}
	return -1;
}

int fcb_free_file_descriptor(FILE_DESCRIPTOR* file_descriptor) {	
	if (file_descriptor->status & FILE_DESCRIPTOR_STATUS_ALLOCATED) {
		if (file_descriptor->status & FILE_DESCRIPTOR_STATUS_OPENED) {
			fcb_close_file_descriptor(file_descriptor);
		}
		memset(file_descriptor->file_name, 0, sizeof(file_descriptor->file_name));
		file_descriptor->status = 0;
		return 0;
	}	
	return 1;
}
int fcb_open_file_descriptor(FILE_DESCRIPTOR* file_descriptors, const char* fn, const char* mode, FILE_DESCRIPTOR** file_descriptor) {
	FILE* file = NULL;
	fopen_s(&file, fn, mode);
	if (file != NULL) {
		FILE_DESCRIPTOR* fd = NULL;
		fcb_alloc_file_descriptor(file_descriptors, fn, &fd);
		if (fd != NULL) {
			get_file_size(fd->file, &fd->file_len);
			fd->file = file;
			fd->status |= FILE_DESCRIPTOR_STATUS_OPENED;
			*file_descriptor = fd;
			return 0;
		}
		else {
			fclose(file);
			file = NULL;
		}
	}
	return 1;
}

int fcb_close_file_descriptor(FILE_DESCRIPTOR* file_descriptor) {	
	if ((file_descriptor->status & FILE_DESCRIPTOR_STATUS_ALLOCATED) != 0 &&
		(file_descriptor->status & FILE_DESCRIPTOR_STATUS_OPENED) != 0) {
		if (file_descriptor->file != NULL) {
			fclose(file_descriptor->file);
			file_descriptor->file = NULL;
		}
		file_descriptor->status &= ~FILE_DESCRIPTOR_STATUS_OPENED;
		return 0;
	}
	return 1; // unallocated or not opened
}
