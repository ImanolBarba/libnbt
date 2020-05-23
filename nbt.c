#include "nbt.h"

void destroyTag(Tag* t);
void destroyTagList(TagList* l);
void destroyTagCompound(TagCompound* tc);
unsigned int parsePayload(void* addr,Tag* t);

void destroyTag(Tag* t) {
    if(t->nameLength) {
        free(t->name);
    }

    if(t->type == TAG_BYTEARRAY || t->type == TAG_INTARRAY || t->type == TAG_LIST) {
        destroyTagList((TagList*)t->payload);
    } else if(t->type == TAG_COMPOUND) {
        destroyTagCompound((TagCompound*)t->payload);
    }

    if(t->payloadLength) {
        free(t->payload);
    }
}

void destroyTagList(TagList* l) {
    for(int i = 0; i < l->size; ++i) {
        destroyTag(&l->list[i]);
    }
}

void destroyTagCompound(TagCompound* tc) {
    for(int i = 0; i < tc->numTags; ++i) {
        destroyTag(&tc->list[i]);
    }
}

size_t getTypeSize(uint8_t type) {
    switch(type) {
        case TAG_BYTE:
            return sizeof(uint8_t);
            break;
        case TAG_SHORT:
            return sizeof(uint16_t);
            break;
        case TAG_INT:
            return sizeof(uint32_t);
            break;
        case TAG_LONG:
            return sizeof(uint64_t);
            break;
        case TAG_FLOAT:
            return sizeof(float);
            break;
        case TAG_DOUBLE:
            return sizeof(double);
            break;
        default:
            break;
    }
    return 0;
}

unsigned int parseList(void* addr, TagList* tl, uint8_t type) {
    void* pos = addr;
    if(type == TAG_LIST) {
        tl->type = *((uint8_t*)pos);
        pos += sizeof(uint8_t);
    } else if(type == TAG_BYTEARRAY) {
        tl->type = TAG_BYTE;
    }  else if(type == TAG_INTARRAY) {
        tl->type = TAG_INT;
    }

    tl->size = __bswap_32(*((uint32_t*)pos));
    pos += sizeof(uint32_t);
    tl->list = NULL;
    if(tl->type != TAG_END) {
        tl->list = calloc(tl->size,sizeof(Tag));
        for(int i = 0; i < tl->size; ++i) {
            Tag *t = &tl->list[i];
            t->type = tl->type;
            t->name = NULL;
            t->nameLength = 0;
            pos += parsePayload(pos,t);
        }
    }
    return pos - addr;
}

unsigned int parseCompound(void* addr, TagCompound* tc) {
    void* pos = addr;
    unsigned int numTags = 0;
    Tag* list = calloc(REALLOC_SIZE,sizeof(Tag));
    do {
        if(numTags && !(numTags % REALLOC_SIZE)) {
            void* newptr = reallocarray(list, numTags + REALLOC_SIZE, sizeof(Tag));
            if(!newptr) {
                fprintf(stderr,"Unable to request memory realloc\n");
                break;
            }
            list = newptr;
        }
        pos += parseTag(pos,&list[numTags]);
    } while(list[numTags++].type != TAG_END);

    tc->list = list;
    tc->numTags = numTags-1;
    return pos - addr;
}

unsigned int parsePayload(void* addr,Tag* t) {
    void* pos = addr;
    t->payloadLength = getTypeSize(t->type); // initially, then particularly for lists/compounds/strings
    TagCompound* tc;
    TagList *tl;
    switch(t->type) {
        case TAG_BYTE:
        case TAG_SHORT:
        case TAG_INT:
        case TAG_LONG:
        case TAG_FLOAT:
        case TAG_DOUBLE:
            t->payload = calloc(1,t->payloadLength);
            memcpy(t->payload,pos,t->payloadLength);
            pos += t->payloadLength;
            break;
        case TAG_STRING:
            t->payloadLength = __bswap_16(*((uint16_t*)pos));
            t->payload = NULL;
            if(t->payloadLength) {
                t->payload = calloc(t->payloadLength,sizeof(char));
                memcpy(t->payload,pos+sizeof(uint16_t),t->payloadLength);
            }
            pos += sizeof(uint16_t) + t->payloadLength;
            break;
        case TAG_COMPOUND:
            tc = (TagCompound*)calloc(1,sizeof(TagCompound));
            t->payloadLength = sizeof(sizeof(TagCompound));
            t->payload = tc;
            pos += parseCompound(pos,tc);
            break;
        case TAG_LIST:
        case TAG_BYTEARRAY:
        case TAG_INTARRAY:
            tl = (TagList*)calloc(1,sizeof(TagList));
            t->payloadLength = sizeof(sizeof(TagList));
            t->payload = tl;
            pos += parseList(pos,tl,t->type);
            break;
    }
    return pos - addr;
}

unsigned int parseTag(void* addr, Tag* t) {
    void* pos = addr;
    t->type = *((uint8_t*)pos);
    t->nameLength = 0;
    t->payloadLength = 0;
    pos += sizeof(uint8_t);
    if(t->type != TAG_END) {
        t->nameLength = __bswap_16(*((uint16_t*)pos));
        t->name = NULL;
        if(t->nameLength) {
            t->name = calloc(t->nameLength,sizeof(char));
            memcpy(t->name,pos+sizeof(uint16_t),t->nameLength);
        }
        pos += sizeof(uint16_t) + t->nameLength;
    }
    pos += parsePayload(pos,t);

    return pos-addr;
}