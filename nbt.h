#ifndef _NBT_H
#define _NBT_H

#include <stdint.h>
#include <byteswap.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <zlib.h>

#ifndef REALLOC_SIZE
#define REALLOC_SIZE 10
#endif

#ifndef GZIP_BUFFER
#define GZIP_BUFFER 4096
#endif

#define GZIP_MAGIC 0x8B1F

typedef struct Tag {
    uint8_t type;
    char* name;
    uint16_t nameLength;
    unsigned int payloadLength;
    void* payload;
} Tag;

typedef struct TagList {
    uint8_t type;
    uint32_t size;
    Tag* list;
} TagList;

typedef struct TagCompound {
    unsigned int numTags;
    Tag* list;
} TagCompound;

enum TAG {
    TAG_END = 0,
    TAG_BYTE,
    TAG_SHORT,
    TAG_INT,
    TAG_LONG,
    TAG_FLOAT,
    TAG_DOUBLE,
    TAG_BYTEARRAY,
    TAG_STRING,
    TAG_LIST,
    TAG_COMPOUND,
    TAG_INTARRAY
};

ssize_t loadDB(const char* filename, void** data);
void destroyTag(Tag* t);
unsigned int parseTag(void* addr, Tag* t);
size_t composeTag(Tag t, void** data);

#endif