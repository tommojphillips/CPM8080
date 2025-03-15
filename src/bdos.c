/* bdos.h
 * Github: https:\\github.com\tommojphillips
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <conio.h>
#include <io.h>

#include "file.h"
#include "cpm.h"
#include "i8080.h"
#include "fcb.h"

static void snth() {
	/* Search for nth */
	int find_result = 0;
	while ((find_result = _findnext(cpm.h_file, &cpm.c_file)) == 0) {
		if (cpm.c_file.attrib & _A_SUBDIR) continue;
		if (fcb_is_valid_fn(cpm.c_file.name) == 0) break;
	}

	if (find_result == 0) {
		fcb_fill_info((FILE_CONTROL_BLOCK*)&cpm.memory[cpm.dma_address], cpm.c_file.name, cpm.c_file.size);
		cpm.file_searching_flag = 1;
		cpm.cpu.registers[REG_A] = 0;
	}
	else {
		_findclose(cpm.h_file);
		cpm.file_searching_flag = 0;
		cpm.cpu.registers[REG_A] = 0xFF;
	}
}

/* BDOS Functions */

void bdos_p_termcpm() {
	/* BDOS function 0 (P_TERMCPM) - System Reset */
	fprintf(stderr, "\nCPM program terminated\n");
	cpm.cpu.flags.halt = 1;
}
static void bdos_c_read() {
	/* BDOS function 1 (C_READ) - Console input */
	cpm.cpu.registers[REG_A] = (char)_getch();
	_putch(cpm.cpu.registers[REG_A]);
	if (cpm.cpu.registers[REG_A] == 0x03) {
		cpm.cpu.flags.halt = 1;
	}
}
static void bdos_c_write() {
	/* BDOS function 2 (C_WRITE) - Console output */
	_putch(cpm.cpu.registers[REG_E]);
}
static void bdos_c_rawio() {
	/* BDOS function 6 (C_RAWIO) - Direct console I/O */
	uint16_t e = cpm.cpu.registers[REG_E];
	cpm.cpu.registers[REG_A] = 0;
	switch (e) {
		case 0xFF:
			if (_kbhit()) {
				cpm.cpu.registers[REG_A] = (char)_getch();
			}
			break;

		case 0xFE:
			if (_kbhit()) {
				cpm.cpu.registers[REG_A] = 0xFF;
			}
			break;

		case 0xFD:
			cpm.cpu.registers[REG_A] = (char)_getch();
			break;

		default:
			_putch(e);
			break;
	}
}
static void bdos_c_writestr() {
	/* BDOS function 9 (C_WRITESTR) - Output string */
	uint16_t i = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	uint8_t b = cpm.cpu.read_byte(i++);
	while (b != cpm.str_delimiter) {
		_putch(b);
		b = cpm.cpu.read_byte(i++);
	}
}
static void bdos_c_readstr() {
	/* BDOS function 10 (C_READSTR) - Buffered console input */
	uint16_t de = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	uint16_t address = de;

	if (de == 0) {
		address = cpm.dma_address;
	}

	typedef struct {
		uint8_t size;
		uint8_t len;
		uint8_t* ptr;
	} BUFFER;
	BUFFER* buffer = (BUFFER*)&cpm.memory[address];
	buffer->ptr = &cpm.memory[address + 2];

	char ch = 0;
	while (1) {
		ch = (char)_getch();
		if (ch == 0x0D || ch == 0x03) {
			break;
		}

		if (ch == 0x08) {
			if (buffer->len > 0) {
				printf("\b \b");
				buffer->ptr[--buffer->len] = 0;
			}
		}
		else {
			if (buffer->size > buffer->len) {
				_putch(ch);
				buffer->ptr[buffer->len++] = ch;
			}
		}
	}
}
static void bdos_c_stat() {
	/* BDOS function 11 (C_STAT) - Console status */
	cpm.cpu.registers[REG_A] = 0;
	if (_kbhit()) {
		cpm.cpu.registers[REG_A] = 0xFF;
	}
	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A];
}
static void bdos_s_bdosver() {
	/* BDOS function 12 (S_BDOSVER) - Return version number */
	uint8_t sys_type = 0;
	cpm.cpu.registers[REG_A] = sys_type;
	cpm.cpu.registers[REG_L] = sys_type;

	uint8_t ver_type = 22;
	cpm.cpu.registers[REG_B] = ver_type;
	cpm.cpu.registers[REG_H] = ver_type;
}
static void bdos_drv_allreset() {
	/* BDOS function 13 (DRV_ALLRESET) - Reset discs */
	//printf("[DRV_ALLRESET]\n");
}
static void bdos_drv_set() {
	/* BDOS function 14 (DRV_SET) - Select disc */
	cpm.selected_drive = cpm.cpu.registers[REG_E];
	cpm.cpu.registers[REG_L] = 0;
	cpm.cpu.registers[REG_A] = 0;
	//printf("[DRV_SET] %c\n", 'A' + cpm.selected_drive);
}
static void bdos_f_open() {
	/* BDOS function 15 (F_OPEN) - Open file */
	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	fcb_get_fn(fcb, cpm.fn_buffer);

	FILE_DESCRIPTOR* fd = NULL;
	fcb_find_file_descriptor(cpm.files, cpm.fn_buffer, &fd);
	if (fd == NULL) {
		fcb_alloc_file_descriptor(cpm.files, cpm.fn_buffer, &fd);
	}

	if (fd != NULL) {
		if ((fd->status & FILE_DESCRIPTOR_STATUS_OPENED) == 0) {
			fopen_s(&fd->file, cpm.fn_buffer, "r+b");
			if (fd->file != NULL) {
				fd->status |= FILE_DESCRIPTOR_STATUS_OPENED;
				get_file_size(fd->file, &fd->file_len);
				cpm.cpu.registers[REG_A] = 0;
			}
			else {
				cpm.cpu.registers[REG_A] = 0xFF;
			}
		}
	}
	else {
		cpm.cpu.registers[REG_A] = 0xFF;
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A];
	printf("[F_OPEN] %s; R=%02X\n", cpm.fn_buffer, cpm.cpu.registers[REG_A]);
}
static void bdos_f_close() {
	/* BDOS function 16 (F_CLOSE) - Close file */
	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	fcb_get_fn(fcb, cpm.fn_buffer);
	FILE_DESCRIPTOR* fd = NULL;
	fcb_find_file_descriptor(cpm.files, cpm.fn_buffer, &fd);
	if (fd != NULL) {
		fcb_free_file_descriptor(fd);
	}
	printf("[F_CLOSE] %s\n", cpm.fn_buffer);
}
static void bdos_f_sfirst() {
	/* BDOS function 17 (F_SFIRST) - search for first */

	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* de_fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	FILE_CONTROL_BLOCK* dma_fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[cpm.dma_address];
	fcb_get_fn(de_fcb, cpm.fn_buffer);

	if (strchr(cpm.fn_buffer, '*') == NULL && strchr(cpm.fn_buffer, '?') == NULL) {
		FILE* file;
		fopen_s(&file, cpm.fn_buffer, "rb");
		if (file == NULL) {
			cpm.cpu.registers[REG_A] = 0xFF;
		}
		else {
			uint32_t file_size = 0;
			get_file_size(file, &file_size);
			fclose(file);
			fcb_fill_info(dma_fcb, cpm.fn_buffer, file_size);
			cpm.cpu.registers[REG_A] = 0;
		}
	}
	else {
		size_t len = strlen(cpm.fn_buffer);
		for (size_t i = 0; i < len; ++i) {
			if (cpm.fn_buffer[i] == '?') {
				cpm.fn_buffer[i] = '*';
				continue;
			}
		}

		cpm.h_file = _findfirst(cpm.fn_buffer, &cpm.c_file);
		if (cpm.h_file == -1L) {
			cpm.cpu.registers[REG_A] = 0xFF;
		}
		else {
			if ((cpm.c_file.attrib & _A_SUBDIR) == 0) {
				if (fcb_is_valid_fn(cpm.c_file.name) == 0) {
					cpm.file_searching_flag = 1;
					fcb_fill_info(dma_fcb, cpm.c_file.name, cpm.c_file.size);
					cpm.cpu.registers[REG_A] = 0;
				}
			}
			else {
				snth();
			}
		}
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A];
}
static void bdos_f_snext() {
	/* BDOS function 18 (F_SNEXT) - search for next */

	if (cpm.file_searching_flag == 0) {
		cpm.cpu.registers[REG_A] = 0xFF;
	}
	else {
		snth();
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A];

}
static void bdos_f_delete() {
	/* BDOS function 19 (F_DELETE) - Delete file */
	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	fcb_get_fn(fcb, cpm.fn_buffer);
	FILE_DESCRIPTOR* fd = NULL;
	fcb_find_file_descriptor(cpm.files, cpm.fn_buffer, &fd);
	if (fd != NULL) {
		fcb_free_file_descriptor(fd);
	}
	
	if (delete_file(cpm.fn_buffer) == 0) {
		cpm.cpu.registers[REG_A] = 0;
	}
	else {
		cpm.cpu.registers[REG_A] = 0xFF;
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A];
	printf("[F_DELETE] %s R=%02X\n", cpm.fn_buffer, cpm.cpu.registers[REG_A]);
}
static void bdos_f_read() {
	/* BDOS function 20 (F_READ) - read next record */
	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	fcb_get_fn(fcb, cpm.fn_buffer);

	uint32_t offset;
	fcb_get_seq_offset(fcb, &offset);

	FILE_DESCRIPTOR* fd = NULL;
	if (fcb_find_file_descriptor(cpm.files, cpm.fn_buffer, &fd) == -1) {
		cpm.cpu.registers[REG_A] = 9; // Invalid fcb
	}
	else if (fseek(fd->file, offset, SEEK_SET) != 0) {
		cpm.cpu.registers[REG_A] = 0xFF; // Hardware error
	}
	else {
		memset(&cpm.memory[cpm.dma_address], 0, RECORD_SIZE);
		fread(&cpm.memory[cpm.dma_address], 1, RECORD_SIZE, fd->file);
		fcb_set_seq_offset(fcb, offset + RECORD_SIZE);

		if (offset >= fd->file_len)
			cpm.cpu.registers[REG_A] = 1;
		else
			cpm.cpu.registers[REG_A] = 0; // OK
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A]; // L = A
}
static void bdos_f_write() {
	/* BDOS function 21 (F_WRITE) - write next record */
	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	fcb_get_fn(fcb, cpm.fn_buffer);

	uint32_t offset;
	fcb_get_seq_offset(fcb, &offset);

	FILE_DESCRIPTOR* fd = NULL;
	if (fcb_find_file_descriptor(cpm.files, cpm.fn_buffer, &fd) == -1) {
		cpm.cpu.registers[REG_A] = 9; // Invalid fcb
	}
	else if (fseek(fd->file, offset, SEEK_SET) != 0) {
		cpm.cpu.registers[REG_A] = 0xFF; // Hardware error
	}
	else {
		fwrite(&cpm.memory[cpm.dma_address], 1, RECORD_SIZE, fd->file);
		fcb_set_seq_offset(fcb, offset + RECORD_SIZE);
		cpm.cpu.registers[REG_A] = 0; // OK
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A]; // L = A
}
static void bdos_f_make() {
	/* BDOS function 22 (F_MAKE) - Create file */
	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	fcb_get_fn(fcb, cpm.fn_buffer);

	FILE* file = NULL;
	fopen_s(&file, cpm.fn_buffer, "w+b");
	if (file == NULL) {
		cpm.cpu.registers[REG_A] = 0xFF;
	}
	else {
		FILE_DESCRIPTOR* fd = NULL;
		fcb_alloc_file_descriptor(cpm.files, cpm.fn_buffer, &fd);
		if (fd == NULL) {
			cpm.cpu.registers[REG_A] = 0x09; // Invalid fcb
		}
		else {
			get_file_size(fd->file, &fd->file_len);
			fd->file = file;
			fd->status |= FILE_DESCRIPTOR_STATUS_OPENED;
			cpm.cpu.registers[REG_A] = 0;
		}
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A];
	printf("[F_MAKE] %s R=%02X\n", cpm.fn_buffer, cpm.cpu.registers[REG_A]);
}
static void bdos_drv_get() {
	/* BDOS function 25 (DRV_GET) - Return current drive */
	cpm.cpu.registers[REG_A] = cpm.selected_drive;
}
static void bdos_f_dmaoff() {
	/* BDOS function 26 (F_DMAOFF) - Set DMA address */
	cpm.dma_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
}
static void bdos_f_usernum() {
	/* BDOS function 32 (F_USERNUM) - get/set user number */
	uint8_t e = cpm.cpu.registers[REG_E];

	if (e == 0xFF) {
		cpm.cpu.registers[REG_A] = cpm.user_num;
	}
	else {
		cpm.user_num = cpm.cpu.registers[REG_E] & 0xE;
	}
}
static void bdos_f_readrand() {
	/* BDOS function 33 (F_READRAND) - Random access read record */
	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	FILE_DESCRIPTOR* fd = NULL;
	uint32_t offset;

	fcb_get_fn(fcb, cpm.fn_buffer);
	fcb_get_rand_offset(fcb, &offset);

	if (fcb_find_file_descriptor(cpm.files, cpm.fn_buffer, &fd) == -1) {
		cpm.cpu.registers[REG_A] = 9;  // Invalid FCB
	}
	else if (fseek(fd->file, offset, SEEK_SET) != 0) {
		cpm.cpu.registers[REG_A] = 3;
	}
	else {
		memset(&cpm.memory[cpm.dma_address], 0, RECORD_SIZE);
		fread(&cpm.memory[cpm.dma_address], 1, RECORD_SIZE, fd->file);
		fcb_set_rand_offset(fcb, offset + RECORD_SIZE);

		if (offset >= fd->file_len)
			cpm.cpu.registers[REG_A] = 1; // Reading unwritten data
		else
			cpm.cpu.registers[REG_A] = 0; // OK
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A]; // A = L	
}
static void bdos_f_writerand() {
	/* BDOS function 34 (F_WRITERAND) - Random access write record */
	uint16_t fcb_address = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	FILE_CONTROL_BLOCK* fcb = (FILE_CONTROL_BLOCK*)&cpm.memory[fcb_address];
	uint32_t offset;
	FILE_DESCRIPTOR* fd = NULL;

	fcb_get_fn(fcb, cpm.fn_buffer);
	fcb_get_rand_offset(fcb, &offset);

	if (fcb_find_file_descriptor(cpm.files, cpm.fn_buffer, &fd) == -1) {
		cpm.cpu.registers[REG_A] = 9; // Invalid FCB
	}
	else if (fseek(fd->file, offset, SEEK_SET) != 0) {
		cpm.cpu.registers[REG_A] = 3;
	}
	else {
		fwrite(&cpm.memory[cpm.dma_address], 1, RECORD_SIZE, fd->file);
		fcb_set_rand_offset(fcb, offset + RECORD_SIZE);
		cpm.cpu.registers[REG_A] = 0; // OK
	}

	cpm.cpu.registers[REG_H] = 0;
	cpm.cpu.registers[REG_L] = cpm.cpu.registers[REG_A]; // A = L
}
static void bdos_c_delimit() {
	/* BDOS function 110 (C_DELIMIT) - Get/set string delimiter */
	uint16_t de = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	if (de == 0xFFFF) {
		cpm.cpu.registers[REG_A] = cpm.str_delimiter;
	}
	else {
		cpm.str_delimiter = cpm.cpu.registers[REG_E];
	}
}

void bdos_function(uint8_t func) {
	switch (func) {
	case 0x00:
		bdos_p_termcpm();
		break;

	case 0x01:
		bdos_c_read();
		break;

	case 0x02:
		bdos_c_write();
		break;

	case 0x06:
		bdos_c_rawio();
		break;

	case 0x09:
		bdos_c_writestr();
		break;

	case 0x0A:
		bdos_c_readstr();
		break;

	case 0x0B:
		bdos_c_stat();
		break;

	case 0x0C:
		bdos_s_bdosver();
		break;

	case 0x0D:
		bdos_drv_allreset();
		break;

	case 0x0E:
		bdos_drv_set();
		break;

	case 0x0F:
		bdos_f_open();
		break;

	case 0x10:
		bdos_f_close();
		break;

	case 0x11:
		bdos_f_sfirst();
		break;

	case 0x12:
		bdos_f_snext();
		break;

	case 0x13:
		bdos_f_delete();
		break;

	case 0x14:
		bdos_f_read();
		break;

	case 0x15:
		bdos_f_write();
		break;

	case 0x16:
		bdos_f_make();
		break;

	case 0x19:
		bdos_drv_get();
		break;

	case 0x1A:
		bdos_f_dmaoff();
		break;

	case 0x20:
		bdos_f_usernum();
		break;

	case 0x21:
		bdos_f_readrand();
		break;

	case 0x22:
		bdos_f_writerand();
		break;

	case 0x6E:
		bdos_c_delimit();
		break;

	default:
		printf("BDOS function %x not implemented\n", func);
		break;
	}
}
