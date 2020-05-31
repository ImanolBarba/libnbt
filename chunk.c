#include "chunk.h"

RegionID translateChunkToRegion(int x, int z);
RegionID translateCoordsToRegion(double x, double y, double z);
ChunkID translateCoordsToChunk(double x, double y, double z);
int overwriteChunk(const char* regionFolder, ChunkID chunk, void* chunkData, size_t chunkLength);
ssize_t loadChunk(const char* regionFolder, ChunkID chunk, void** chunkData);

RegionID translateChunkToRegion(int x, int z) {
    RegionID region;
    region.x = floor((double)x/CHUNKS_PER_REGION);
    region.z = floor((double)z/CHUNKS_PER_REGION);
    return region;
}

RegionID translateCoordsToRegion(double x, double y, double z) {
    ChunkID chunk = translateCoordsToChunk(x,y,z);
    return translateChunkToRegion(chunk.x,chunk.z);
}

ChunkID translateCoordsToChunk(double x, double y, double z) {
    ChunkID chunk;
    chunk.x = floor((double)x/BLOCKS_PER_CHUNK);
    chunk.z = floor((double)z/BLOCKS_PER_CHUNK);
    return chunk;
}

int overwriteChunk(const char* regionFolder, ChunkID chunk, void* chunkData, size_t chunkLength) {
    RegionID region = translateChunkToRegion(chunk.x,chunk.z);
    ChunkID relativeChunk;
    relativeChunk.x = chunk.x & 31;
    relativeChunk.z = chunk.z & 31;

    char* regionFilename = calloc(MAX_REGION_FILENAME_LENGTH + strlen(regionFolder),sizeof(char));
    sprintf(regionFilename,"%s/r.%d.%d.mca",regionFolder,region.x,region.z);

    if(access(regionFilename,R_OK | W_OK) == -1) {
        fprintf(stderr,"Can't access file %s: %s\n",regionFilename,strerror(errno));
        return -1;
    }

    int fd = open(regionFilename,O_RDWR);
    if(fd == -1) {
        fprintf(stderr,"Unable to open region file %s: %s\n",regionFilename,strerror(errno));
        return -2;
    }
    free(regionFilename);

    uint32_t chunkHeaderOffset;
    if(pread(fd,&chunkHeaderOffset,sizeof(uint32_t),(relativeChunk.x + relativeChunk.z * CHUNK_OFFSET_LENGTH) * sizeof(uint32_t)) == -1) {
        close(fd);
        fprintf(stderr,"Unable to read chunk header offset: %s\n",strerror(errno));
        return -3;
    }
    uint32_t totalChunkLength = (chunkHeaderOffset >> 24) * 4096;
    chunkHeaderOffset = (__bswap_32(chunkHeaderOffset & 0x00FFFFFF) >> 8) * CHUNK_SECTOR_SIZE;
    int pos = lseek(fd,chunkHeaderOffset,SEEK_SET);
    if(pos == -1) {
        close(fd);
        fprintf(stderr,"Unable to seek to header offset: %s\n",strerror(errno));
        return -4;
    }

    ChunkHeader header;
    if(pread(fd,&header,sizeof(ChunkHeader),pos) <= 0) {
        close(fd);
        fprintf(stderr,"Unable to read chunk header: %s\n",strerror(errno));
        return -5;
    }
    header.length = __bswap_32(header.length);

    void* compressedChunk;
    ssize_t compressedChunkLength = deflateGzip(chunkData,chunkLength,&compressedChunk,(header.compressionType == COMPRESSION_TYPE_ZLIB));
    if(compressedChunkLength > totalChunkLength) {
        // Haven't determined if we can just allocate a new 4KiB sector for the chunk
        // To avoid corrupting the region, let's just make the function fail and retry on another chunk that has
        // free space at the end
        close(fd);
        free(compressedChunk);
        fprintf(stderr,"Not enough free space to overwrite the chunk.\n\nOriginal chunk size (with padding): %d\nNew chunk size:%d\n",totalChunkLength,(unsigned int)compressedChunkLength);
        return -6;
    }
    header.length = __bswap_32((uint32_t)compressedChunkLength+1);
    if(write(fd,&header,sizeof(ChunkHeader)) <= 0) {
        close(fd);
        free(compressedChunk);
        fprintf(stderr,"Unable to read chunk header: %s\n",strerror(errno));
        return -7;
    }
    ssize_t nWritten = 0;
    size_t totalWritten = 0;

    while((nWritten = write(fd,compressedChunk+totalWritten,compressedChunkLength-totalWritten))) {
        if(nWritten == -1) {
            if(errno == EINTR) {
                continue;
            }
            fprintf(stderr,"Unable to write chunk: %s\n",strerror(errno));
            close(fd);
            free(compressedChunk);
            return -8;
        }
        totalWritten += nWritten;
    }
    close(fd);
    free(compressedChunk);
    return 0;
}

ssize_t loadChunk(const char* regionFolder, ChunkID chunk, void** chunkData) {
    RegionID region = translateChunkToRegion(chunk.x,chunk.z);
    ChunkID relativeChunk;
    relativeChunk.x = chunk.x & 31;
    relativeChunk.z = chunk.z & 31;

    char* regionFilename = calloc(MAX_REGION_FILENAME_LENGTH + strlen(regionFolder),sizeof(char));
    sprintf(regionFilename,"%s/r.%d.%d.mca",regionFolder,region.x,region.z);

    if(access(regionFilename,R_OK) == -1) {
        fprintf(stderr,"Can't access file %s: %s\n",regionFilename,strerror(errno));
        return -1;
    }

    int fd = open(regionFilename,O_RDONLY);
    if(fd == -1) {
        fprintf(stderr,"Unable to open region file %s: %s\n",regionFilename,strerror(errno));
        return -2;
    }
    free(regionFilename);

    uint32_t chunkHeaderOffset;
    if(pread(fd,&chunkHeaderOffset,sizeof(uint32_t),(relativeChunk.x + relativeChunk.z * CHUNK_OFFSET_LENGTH) * sizeof(uint32_t)) == -1) {
        close(fd);
        fprintf(stderr,"Unable to read chunk header offset: %s\n",strerror(errno));
        return -3;
    }
    chunkHeaderOffset = (__bswap_32(chunkHeaderOffset & 0x00FFFFFF) >> 8) * CHUNK_SECTOR_SIZE;
    if(chunkHeaderOffset == 0) {
        // Chunk not present. Hasn't been generated
        close(fd); 
        return 0;
    }

    if(lseek(fd,chunkHeaderOffset,SEEK_SET) == -1) {
        close(fd);
        fprintf(stderr,"Unable to seek to header offset: %s\n",strerror(errno));
        return -4;
    }

    ChunkHeader header;
    if(read(fd,&header,sizeof(ChunkHeader)) <= 0) {
        close(fd);
        fprintf(stderr,"Unable to read chunk header: %s\n",strerror(errno));
        return -5;
    }
    header.length = __bswap_32(header.length);
    ssize_t chunkLength = header.length;
    if(header.compressionType != COMPRESSION_TYPE_ZLIB && header.compressionType != COMPRESSION_TYPE_GZIP) {
        fprintf(stderr, "Invalid compression method. Proably reading the wrong data for the header\n");
        close(fd); 
        return -6;
    }
    if(header.length == 0) {
        fprintf(stderr, "Header length is 0. Probably reading the wrong data for the header\n");
        close(fd); 
        return -7;
    }
    
    void* compressedChunk = calloc(chunkLength,sizeof(char));
    ssize_t nRead = 0;
    size_t totalRead = 0;
    
    while((nRead = read(fd,compressedChunk+totalRead,chunkLength-totalRead))) {
        if(nRead == -1) {
            if(errno == EINTR) {
                continue;
            }
            fprintf(stderr,"Unable to read chunk: %s\n",strerror(errno));
            close(fd);
            free(compressedChunk);
            return -8;
        }
        totalRead += nRead;
    }
    close(fd);

    void *decompressedChunk;
    chunkLength = inflateGzip(compressedChunk,chunkLength,&decompressedChunk,(header.compressionType == COMPRESSION_TYPE_ZLIB));
    
    if(chunkLength <= 0) {
        fprintf(stderr,"Error while decompressing chunk\n");
        free(compressedChunk);
        return -9;
    }
    free(compressedChunk);

    *chunkData = decompressedChunk;
    return chunkLength;
}