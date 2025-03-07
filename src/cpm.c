/* cpm.c
 * Github: https:\\github.com\tommojphillips
 */

#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "file.h"
#include "cpm.h"
#include "i8080.h"

CPM cpm = { 0 };

#define REFRESH_RATE 60
//#define CPU_CLOCK 2000000 /* 2 Mhz */
#define CPU_CLOCK 1000000000 /* 100 Mhz */
#define VBLANK_RATE (CPU_CLOCK / REFRESH_RATE)

void inject_bdos_signals() {

	// inject "out 0,a" at 0x0000 (signal to stop the test)
	cpm.memory[0x0000] = 0xD3; /* OUT opcode */
	cpm.memory[0x0001] = 0x00; /* PORT 0 */
	cpm.memory[0x0002] = 0x76; /* HLT */

	// inject "out 1,a" at 0x0005 (signal to output some characters)
	cpm.memory[0x0005] = 0xD3; /* OUT opcode */
	cpm.memory[0x0006] = 0x01; /* PORT 1 */
	cpm.memory[0x0007] = 0xC9; /* RET */
}

static void p_term_cpm() {
	fprintf(stderr, "\nCPM program terminated\n");
}
static void c_write_char() {
	fprintf(stderr, "%c", cpm.cpu.registers[REG_E]);
}
static void c_write_str() {
	uint16_t i = (cpm.cpu.registers[REG_D] << 8) | cpm.cpu.registers[REG_E];
	uint8_t b = cpm.cpu.read_byte(i++);
	while (b != '$') {
		fprintf(stderr, "%c", b);
		b = cpm.cpu.read_byte(i++);
	}
}

static void bdos_function(uint8_t func) {
	switch (func) {
		case 0x00:
			p_term_cpm();
			break;

		case 0x02:
			c_write_char();
			break;

		case 0x09:
			c_write_str();
			break;

		default:
			printf("BDOS function %x not implemented\n", func);
			break;
	}
}

static void cpu_tick(uint32_t cycles) {
	cpm.cpu.cycles = 0;
	while (cpm.cpu.cycles < cycles) {
		i8080_execute(&cpm.cpu);
	}
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
			p_term_cpm();
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
	fprintf(stderr, "RESET\n");
}
int cpm_update() {
	cpu_tick(VBLANK_RATE);
	return cpm.cpu.flags.halt;
}

int cpm_init() {
	cpm.memory = (uint8_t*)malloc(0x10000);
	if (cpm.memory == NULL) {
		printf("Failed to allocate ROM\n");
		return 1;
	}
	memset(cpm.memory, 0, 0x10000);

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
}

void cpm_vblank() {
}
