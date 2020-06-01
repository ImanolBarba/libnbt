#include "compression.h"

ssize_t inflateGzip(void* compData, size_t compDataLen, void** unCompData, int headerless) {
    unsigned int increase = compDataLen/2;
    unsigned int uncompLength = compDataLen; // Later to be increased

    char* uncomp = (char*)calloc(uncompLength, sizeof(char));
  
    z_stream strm;
    strm.next_in = (Bytef*) compData;
    strm.avail_in = compDataLen;
    strm.total_out = 0;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
  
    int err = Z_OK;
    if(headerless) {
        err = inflateInit(&strm);
    } else {
        err = inflateInit2(&strm, 16+MAX_WBITS);
    }

    if(err != Z_OK) {
        free(uncomp);
        return ZLIB_STREAM_INIT_ERROR;
    }
  
    do {
        // If our output buffer is too small
        if(strm.total_out >= uncompLength ) {
            // Increase size of output buffer
            void* newptr = realloc(uncomp,uncompLength + increase);
            if(newptr == NULL) {
                inflateEnd(&strm);
                free(uncomp);
                return MEMORY_ERROR;
            }
            uncomp = newptr;
            uncompLength += increase;
        }
  
        strm.next_out = (Bytef *) (uncomp + strm.total_out);
        strm.avail_out = uncompLength - strm.total_out;

        // Inflate another chunk.
        err = inflate(&strm, Z_SYNC_FLUSH);
        if(err != Z_OK && err != Z_STREAM_END) {
            inflateEnd(&strm);
            free(uncomp);
            return ZLIB_INFLATE_ERROR;
        }
    } while(err != Z_STREAM_END);
    uncompLength = strm.total_out;

    if(inflateEnd(&strm) != Z_OK) {
        free(uncomp);
        return ZLIB_STREAM_FREE_ERROR;
    }

    *unCompData = uncomp;
    return uncompLength;
}

ssize_t deflateGzip(void* unCompData, size_t unCompDataLen, void** compData, int headerless) {
    unsigned int compLength = unCompDataLen;
    unsigned int increase = compLength/4;
    char* comp = (char*)calloc(compLength, sizeof(char));
    
    z_stream strm;
    strm.next_in = (Bytef*) unCompData;
    strm.avail_in = unCompDataLen;
    strm.total_out = 0;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;

    int err = Z_OK;
    if(headerless) {
        err = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    } else {
        err = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16+MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
    }
    
    if(err != Z_OK) {
        free(comp);
        return ZLIB_STREAM_INIT_ERROR;
    }

    do {
        // If our output buffer is too small
        if (strm.total_out >= compLength) {
            // Increase size of output buffer
            void* newptr = realloc(comp, compLength + increase);
            if(newptr == NULL) {
                deflateEnd(&strm);
                free(comp);
                return MEMORY_ERROR;
            }
            comp = newptr;
            compLength += increase;
        }

        strm.next_out = (Bytef*) (comp + strm.total_out);
        strm.avail_out = compLength - strm.total_out;

        // deflate another chunk
        err = deflate(&strm, Z_FINISH);
        if(err != Z_OK && err != Z_STREAM_END) {
            deflateEnd(&strm);
            free(comp);
            return ZLIB_DEFLATE_ERROR;
        }
    } while(err != Z_STREAM_END);
    compLength = strm.total_out;
    
    if(deflateEnd(&strm) != Z_OK) {
        free(comp);
        return ZLIB_STREAM_FREE_ERROR;
    }

    if(!headerless) {
        // Set OS Flag to 0x00: "FAT filesystem (MS-DOS, OS/2, NT/Win32)"
        comp[OS_FLAG_OFFSET] = 0x00;
    }

    *compData = comp;
    return compLength;
}