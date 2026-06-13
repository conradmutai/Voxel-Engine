#include "chunkserializer.h"
#include "chunk.h"
#include "utils.h"

#include <iostream>

ChunkSerializer::ChunkSerializer() {
    m_saveDirectory = getExecutableDir() + "saves/chunks/";
}

ChunkSerializer::~ChunkSerializer() {

}

bool ChunkSerializer::save(Chunk* chunk, glm::ivec2 chunkPos) {
    std::string filePath = getFilePath(chunkPos);
    FILE* file = std::fopen(filePath.c_str(), "wb");

    if (file == nullptr) {
        return false;
    }

    int xCoord = chunkPos.x;
    int zCoord = chunkPos.y;
    size_t size = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH;

    std::fwrite(&xCoord, sizeof(int), 1, file);
    std::fwrite(&zCoord, sizeof(int), 1, file);
    std::fwrite(&size, sizeof(size_t), 1, file);

    std::vector<uint8_t> compressed = compress(chunk->getBlockData(), size);

    std::fwrite(compressed.data(), sizeof(uint8_t), compressed.size(), file);
    std::fclose(file);

    return true;
}

bool ChunkSerializer::load(Chunk* chunk, glm::ivec2 chunkPos) {
    std::string filePath = getFilePath(chunkPos);
    FILE* file = std::fopen(filePath.c_str(), "rb");

    if (file == nullptr) {
        return false;
    }

    int xCoord, zCoord;
    size_t size;

    fread(&xCoord, 1, sizeof(int), file);
    fread(&zCoord, 1, sizeof(int), file);
    fread(&size, 1, sizeof(size), file);

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    size_t headerSize = sizeof(int) + sizeof(int) + sizeof(size_t);
    size_t compressedSize = fileSize - headerSize;
    fseek(file, headerSize, SEEK_SET);

    std::vector<uint8_t> compressed(compressedSize);
    fread(compressed.data(), sizeof(uint8_t), compressedSize, file);

    size_t originalSize = size;

    std::vector<uint8_t> decompressed = decompress(compressed, originalSize);
    chunk->setBlockData(decompressed.data(), originalSize);

    std::fclose(file);
    
    return true;
}

std::string ChunkSerializer::getFilePath(glm::ivec2 chunkPos) {
    int xCoord = chunkPos.x;
    int zCoord = chunkPos.y;

    std::string xCoordStr = std::to_string(xCoord);
    std::string zCoordStr = std::to_string(zCoord);

    return m_saveDirectory + "chunk_" + xCoordStr + "_" + zCoordStr + ".bin";
}

std::vector<uint8_t> ChunkSerializer::compress(const uint8_t* data, size_t size) {
    std::vector<uint8_t> output;
    
    // bit packing state
    uint8_t currentByte = 0;
    int bitPos = 0;
    
    // helper lambda to write a single bit
    auto writeBit = [&](int bit) {
        currentByte |= (bit & 1) << (7 - bitPos);
        bitPos++;
        if (bitPos == 8) {
            output.push_back(currentByte);
            currentByte = 0;
            bitPos = 0;
        }
    };
    
    // helper lambda to write N bits from a value
    auto writeBits = [&](uint64_t value, int numBits) {
        for (int i = numBits - 1; i >= 0; i--) {
            writeBit((value >> i) & 1);
        }
    };
    
    // flush any remaining bits at the end
    auto flush = [&]() {
        if (bitPos > 0) {
            output.push_back(currentByte);
            currentByte = 0;
            bitPos = 0;
        }
    };
    
    size_t i = 0;
    while (i < size) {
        // count run of identical bytes
        size_t runLength = 1;
        while (i + runLength < size && 
               data[i + runLength] == data[i] && 
               runLength < 127) {
            runLength++;
        }
        
        // threshold: repeated run must be at least 3 bytes to be worth encoding
        if (runLength >= 3) {
            // signal bit = 1 (repeated run)
            writeBit(1);
            // run value (1 bit: 0 or non-zero simplified, use full byte approach)
            writeBit(data[i] > 0 ? 1 : 0);
            // 7-bit run length
            writeBits(runLength, 7);
            // actual value stored as full byte after the header bits
            writeBits(data[i], 8);
            i += runLength;
        } else {
            // find how many bytes until next long repeated run
            size_t rawLength = 0;
            size_t j = i;
            while (j < size && rawLength < 255) {
                // peek ahead for a repeated run
                size_t peekRun = 1;
                while (j + peekRun < size && 
                       data[j + peekRun] == data[j] && 
                       peekRun < 127) {
                    peekRun++;
                }
                if (peekRun >= 3) break; // found a run worth encoding
                rawLength++;
                j++;
            }
            
            if (rawLength == 0) rawLength = 1;
            
            // signal bit = 0 (raw run)
            writeBit(0);
            // 8-bit raw run length
            writeBits(rawLength, 8);
            // raw bytes
            for (size_t k = 0; k < rawLength; k++) {
                writeBits(data[i + k], 8);
            }
            i += rawLength;
        }
    }
    
    flush();
    return output;
}

std::vector<uint8_t> ChunkSerializer::decompress(const std::vector<uint8_t>& compressedData, size_t originalSize) {
    std::vector<uint8_t> output;
    output.reserve(originalSize);
    
    // bit reading state
    size_t bytePos = 0;
    int bitPos = 0;
    
    auto readBit = [&]() -> int {
        if (bytePos >= compressedData.size()) return 0;
        int bit = (compressedData[bytePos] >> (7 - bitPos)) & 1;
        bitPos++;
        if (bitPos == 8) {
            bitPos = 0;
            bytePos++;
        }
        return bit;
    };
    
    auto readBits = [&](int numBits) -> uint64_t {
        uint64_t value = 0;
        for (int i = numBits - 1; i >= 0; i--) {
            value |= (uint64_t)readBit() << i;
        }
        return value;
    };
    
    while (output.size() < originalSize) {
        int signal = readBit();
        
        if (signal == 1) {
            // repeated run
            int isNonZero = readBit();
            size_t runLength = (size_t)readBits(7);
            uint8_t value = (uint8_t)readBits(8);
            
            for (size_t k = 0; k < runLength && output.size() < originalSize; k++) {
                output.push_back(value);
            }
        } else {
            // raw run
            size_t rawLength = (size_t)readBits(8);
            for (size_t k = 0; k < rawLength && output.size() < originalSize; k++) {
                output.push_back((uint8_t)readBits(8));
            }
        }
    }
    
    return output;
}