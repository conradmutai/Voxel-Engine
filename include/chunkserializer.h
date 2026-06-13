#pragma once
#define CHUNKSERIALIZER_H

#include "chunk.h"

struct Marker {
    bool signal;   // 1 bit — 0 = raw run, 1 = repeated run
    
    // if signal == 0 (raw run)
    uint8_t rawRunLength;           // 8 bits
    std::vector<uint8_t> rawData;   // rawRunLength bytes
    
    // if signal == 1 (repeated run)
    bool runValue;      // 1 bit
    uint8_t runLength;  // 7 bits (max 127)
};

class ChunkSerializer {
    public:
        ChunkSerializer();
        ~ChunkSerializer();

        bool save(Chunk* chunk, glm::ivec2 chunkPos);
        bool load(Chunk* chunk, glm::ivec2 chunkPos);
    private:
        std::string m_saveDirectory;
        
        // helper function
        std::string getFilePath(glm::ivec3 chunkPos);

        // compression and decompression algorithms
        std::vector<uint8_t> compress(const uint8_t data, size_t size);
        std::vector<uint8_t> decompress(const std::vector<uint8_t>& compress, size_t size);
};