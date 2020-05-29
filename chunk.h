#ifndef _CHUNK_H
#define _CHUNK_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <byteswap.h>

#include "compression.h"

#define BLOCKS_PER_CHUNK 16
#define CHUNKS_PER_REGION 32
#define CHUNK_OFFSET_LENGTH 32
#define CHUNK_SECTOR_SIZE 4096

// 58593 is the maximum number of regions containing 32 chunks in any direction
// Thus, biggest filename is:
// r.-58593.-58593.mca
#define MAX_REGION_FILENAME_LENGTH 32 // Just to round up

enum COMPRESSION_TYPE {
    COMPRESSION_TYPE_GZIP = 1,
    COMPRESSION_TYPE_ZLIB = 2
};

typedef struct RegionID {
    int x;
    int z;
} RegionID;

typedef struct ChunkID {
    int x;
    int z;
} ChunkID;

typedef struct __attribute__((packed)) ChunkHeader {
    uint32_t length;
    uint8_t compressionType;
} ChunkHeader;

ChunkID translateCoordsToChunk(double x, double y, double z);
int overwriteChunk(const char* regionFolder, ChunkID chunk, void* chunkData, size_t chunkLength);
ssize_t loadChunk(const char* regionFolder, ChunkID chunk, void** chunkData);

#endif