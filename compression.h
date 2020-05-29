#ifndef _COMPRESSION_H
#define _COMPRESSION_H

#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>

#define OS_FLAG_OFFSET 0x9

ssize_t deflateGzip(void* unCompData, size_t unCompDataLen, void** compData, int headerless);
ssize_t inflateGzip(void* compData, size_t compDataLen, void** unCompData, int headerless);

#endif