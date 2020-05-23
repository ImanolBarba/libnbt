#ifndef _NBT_H
#define _NBT_H

#include <stdint.h>
#include <byteswap.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#ifndef REALLOC_SIZE
#define REALLOC_SIZE 10
#endif

typedef struct Tag {
    uint8_t type;
    char* name;
    unsigned int nameLength;
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

void destroyTag(Tag* t);
unsigned int parseTag(void* addr, Tag* t);

#endif