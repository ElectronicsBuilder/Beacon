#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>

void DELAY_INIT(void);
void DELY_MICRO(uint32_t micros);
void DELAY_MILLI(uint32_t millis);

#endif
