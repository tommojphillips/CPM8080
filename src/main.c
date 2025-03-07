/* main.c
 * Github: https:\\github.com\tommojphillips
 */

#include <stdio.h>
#include <string.h>

#include "file.h"
#include "cpm.h"

void args(int argc, char** argv) {
	const char* filename = NULL;
	uint32_t offset = 0x100;
	for (int i = 1; i < argc; ++i) {
		uint32_t len = (uint32_t)strlen(argv[i]);
		int next_arg = 0;
		for (int j = 0; j < (int)len;) {
			switch (argv[i][j]) {
			case 'f':
				read_file_into_buffer(argv[i] + j + 1, cpm.memory, 0x10000, offset, &len, 0);
				offset += len;
				next_arg = 1;
				break;

			case '-':
			case '/':
			default:
				++j;
				break;
			}
			if (next_arg) break;
		}
	}
}

int main(int argc, char** argv) {
	if (cpm_init() != 0) {
		return 1;
	}
	args(argc, argv);
	while (1) {
		if (cpm_update() == 1) {
			break;
		}
	}
	cpm_destroy();
	return 0;
}
