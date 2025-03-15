/* main.c
 * Github: https:\\github.com\tommojphillips
 */

#include <stdio.h>
#include <string.h>

#include "file.h"
#include "cpm.h"

void args(int argc, char** argv) {
	if (argc > 1) {
		size_t len = strlen(argv[1]);
		size_t i = 0;
		for (; i < len; ++i) {
			if (argv[1][i] == '.') {
				++i;
				break;
			}
		}

		char ext[4] = { 0 };
		int j = 0;
		for (; i < len; ++i) {
			if (argv[1][i + j] >= 'a' && argv[1][i + j] <= 'z')
				ext[j] = argv[1][i] - 0x20;
			else
				ext[j] = argv[1][i];
			++j;
		}

		if (strcmp(ext, "COM") == 0) {
			cpm_load_com(argv[1]);
		}
		else if (strcmp(ext, "HEX") == 0) {
			cpm_load_hex(argv[1]);
		}
		else {
			printf("Unknown File type, dont know how to load .%s\n", ext);
		}
	}

	if (argc > 2) {
		cpm_load_params(argv[2]);
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
