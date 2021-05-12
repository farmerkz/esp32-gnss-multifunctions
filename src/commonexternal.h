#include "main.h"
#ifndef __commonexternal_h__
#define __commonexternal_h__

extern size_t logging(const char *ft, bool sdmute = true);
extern size_t logging(const char *ft, uint32_t var, bool sdmute = true);
extern size_t logging(const char *ft, const char *var, bool sdmute = true);

extern void fatalError(uint8_t _n = LED_RED_FATAL_BLINK);

#endif