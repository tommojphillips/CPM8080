/* cpm.h
 * Github: https:\\github.com\tommojphillips
 */

#include <stdint.h>

#include "i8080.h"

#ifndef CPM_H
#define CPM_H

typedef struct {
	I8080 cpu;
	uint8_t* memory;
} CPM;

#ifdef __cplusplus
extern "C" {
#endif

extern CPM cpm;
int cpm_init();
void cpm_destroy();
void cpm_reset();
int cpm_update();
	
#ifdef __cplusplus
};
#endif
#endif