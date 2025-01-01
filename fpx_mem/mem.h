#ifndef FPX_MEM_H
#define FPX_MEM_H

#include <stddef.h>
#include <stdint.h>

extern void* fpx_memcpy(void* dst, void* src, size_t count);
extern void* fpx_memset(void* target, uint8_t value, size_t count);

#endif // FPX_MEM_H
