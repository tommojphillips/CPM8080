/* cpm.c
 * Github: https:\\github.com\tommojphillips
 */

#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <io.h>

#include "file.h"
#include "cpm.h"
#include "bdos.h"
#include "i8080.h"

CPM cpm = { 0 };

// Clock
#define REFRESH_RATE   60
#define CPU_CLOCK      2000000 /* 2 Mhz */
#define VBLANK_RATE    (CPU_CLOCK / REFRESH_RATE)

// Zero page
#define IOBYTE_ADDRESS         0x0003
#define BDOS_ENTRY_ADDRESS     0x0005
#define FCB1_ADDRESS           0x005C
#define FCB2_ADDRESS           0x006C
#define SYSTEM_PARAMS_ADDRESS  0x0080

void load_system_params(const char* arg) {
	int8_t len = strlen(arg) & 0x7E;
#if 0
	cpm.memory[SYSTEM_PARAMS_ADDRESS] = len;
	cpm.memory[SYSTEM_PARAMS_ADDRESS + 1] = ' ';
	memcpy(&cpm.memory[SYSTEM_PARAMS_ADDRESS + 2], arg, len);
#else
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[SYSTEM_PARAMS_ADDRESS];
	memcpy(&fcb->fn, arg, len);
#endif
}

static void inject_bdos_signals() {

	// inject "out 0,a" at 0x0000 (term program)
	cpm.memory[0x0000] = 0xD3; /* OUT opcode */
	cpm.memory[0x0001] = 0x00; /* PORT 0 */
	cpm.memory[0x0002] = 0xC9; /* RET */

	// inject "out 1,a" at 0x0005 (bdos function)
	cpm.memory[0x0005] = 0xD3; /* OUT opcode */
	cpm.memory[0x0006] = 0x01; /* PORT 1 */
	cpm.memory[0x0007] = 0xC9; /* RET */
}

uint8_t cpm_read_byte(uint16_t address) {
	return *(uint8_t*)(cpm.memory + (address & 0xFFFF));
}
void cpm_write_byte(uint16_t address, uint8_t value) {
	*(uint8_t*)(cpm.memory + (address & 0xFFFF)) = value;
}
uint8_t cpm_read_io(uint8_t port) {
	(void)port;
	return 0;
}
void cpm_write_io(uint8_t port, uint8_t value) {
	switch (port) {	
		case 0x00:
			bdos_p_termcpm();
			break;

		case 0x01:
			bdos_function(cpm.cpu.registers[REG_C]);
			break;

		default:
			printf("Writing to undefined port: %02X = %02X\n", port, value);
			break;
	}
}

void cpm_reset() {
	i8080_reset(&cpm.cpu);
	cpm.cpu.pc = 0x100;
	cpm.cpu.sp = 0xFF00;
	cpm.str_delimiter = '$';
	cpm.dma_address = SYSTEM_PARAMS_ADDRESS;
	cpm.fcb1 = (FILE_CONTROL_BLOCK*)(cpm.memory + FCB1_ADDRESS);
	cpm.fcb2 = (FILE_CONTROL_BLOCK*)(cpm.memory + FCB2_ADDRESS);
}
int cpm_update() {
	cpm.cpu.cycles = 0;
	while (!cpm.cpu.flags.halt && cpm.cpu.cycles < VBLANK_RATE) {
		i8080_execute(&cpm.cpu);
	}
	return cpm.cpu.flags.halt;
}

int cpm_init() {
	cpm.memory = (uint8_t*)malloc(0x10000);
	if (cpm.memory == NULL) {
		printf("Failed to allocate CPU Memory\n");
		return 1;
	}
	memset(cpm.memory, 0, 0x10000);

	cpm.files = (FILE_DESCRIPTOR*)malloc(sizeof(FILE_DESCRIPTOR) * MAX_FILE_DESCRIPTORS);
	if (cpm.files == NULL) {
		printf("Failed to allocate File Descriptors\n");
		return 1;
	}
	memset(cpm.files, 0, sizeof(FILE_DESCRIPTOR) * MAX_FILE_DESCRIPTORS);

	cpm.disks = (DISK_DESCRIPTOR*)malloc(sizeof(DISK_DESCRIPTOR) * MAX_DISK_DESCRIPTORS);
	if (cpm.disks == NULL) {
		printf("Failed to allocate Disk Descriptors\n");
		return 1;
	}
	memset(cpm.disks, 0, sizeof(DISK_DESCRIPTOR) * MAX_DISK_DESCRIPTORS);

	i8080_init(&cpm.cpu);
	cpm.cpu.read_byte = cpm_read_byte;
	cpm.cpu.write_byte = cpm_write_byte;
	cpm.cpu.read_io = cpm_read_io;
	cpm.cpu.write_io = cpm_write_io;

	cpm_reset();
	inject_bdos_signals();
	return 0;
}
void cpm_destroy() {
	if (cpm.memory != NULL) {
		free(cpm.memory);
		cpm.memory = NULL;
	}

	if (cpm.files != NULL) {
		for (int i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
			if ((cpm.files[i].status & FILE_DESCRIPTOR_STATUS_ALLOCATED) &&
				(cpm.files[i].status & FILE_DESCRIPTOR_STATUS_OPENED) &&
				cpm.files[i].file != NULL) {
				fclose(cpm.files[i].file);
				cpm.files[i].file = NULL;
			}
		}
		free(cpm.files);
		cpm.files = NULL;
	}

	if (cpm.disks != NULL) {
		free(cpm.disks);
		cpm.disks = NULL;
	}
}

void cpm_load_com(const char* arg) {
	read_file_into_buffer(arg, cpm.memory, 0x10000, 0x100, NULL, 0);
}
void cpm_load_hex(const char* arg) {

	char buf[128] = { 0 };
	char* line_ptr = NULL;
	char* value_ptr = NULL;
	char* next_token = NULL;

	FILE* file;
	fopen_s(&file, arg, "rb");
	if (file == NULL) {
		return;
	}

	// parse next line
	while (fgets(buf, 128, file) != NULL) {
		line_ptr = buf;
		value_ptr = strtok_s(line_ptr, ":", &next_token);

		char hex[3] = { 0 };
		uint32_t i = 0;
		while (value_ptr[0] != '\r') {
			memcpy(hex, value_ptr, 2);
			cpm.memory[0x100 + i] = (uint8_t)strtol(hex, NULL, 16);
			printf("%02X ", cpm.memory[0x100 + i]);
			value_ptr++;
			value_ptr++;
			i++;
		}
		printf("\n");
	}

	fclose(file);
	file = NULL;
}
void cpm_load_params(const char* arg) {
	load_system_params(arg);
	fcb_fill_info(cpm.fcb1, arg, 0);
}
