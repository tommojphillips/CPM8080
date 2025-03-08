/* main.c
 * Github: https:\\github.com\tommojphillips
 */

#include <stdio.h>
#include <string.h>

#include "file.h"
#include "cpm.h"

int args(int argc, char** argv) {
	for (int i = 1; i < argc; ++i) {
		return read_file_into_buffer(argv[i], cpm.memory, 0x10000, 0x100, NULL, 0);
	}
	return 0;
}

int main(int argc, char** argv) {
	if (cpm_init() != 0) {
		return 1;
	}
	if (args(argc, argv) != 0) {
		return 1;
	}	
	while (1) {
		if (cpm_update() == 1) {
			break;
		}
	}
	cpm_destroy();
	return 0;
}
