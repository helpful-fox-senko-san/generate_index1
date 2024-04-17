/* this file is in the public domain */

#ifndef _CRC32_H
#define _CRC32_H

#include <stdint.h>

typedef struct {
  unsigned int crc;
} CRC32_CTX;

void CRC32_Init(CRC32_CTX *context);
void CRC32_Update(CRC32_CTX *context, const void *data, unsigned long len);
uint32_t CRC32_Final(CRC32_CTX *context);

uint32_t CRC32(
    const char *str,
    unsigned long len);

#endif /* _CRC32_H */
