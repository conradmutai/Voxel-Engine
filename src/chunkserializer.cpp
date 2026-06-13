#include "chunkserializer.h"

ChunkSerializer::ChunkSerializer() {

}

ChunkSerializer::~ChunkSerializer() {

}

bool ChunkSerializer::save(Chunk* chunk, glm::ivec2 chunkPos) {

}

bool ChunkSerializer::load(Chunk* chunk, glm::ivec2 chunkPos) {

}

std::string ChunkSerializer::getFilePath(glm::ivec2 chunkPos) {

}

std::vector<uint8_t> ChunkSerializer::compress(const uint8_t data, size_t size) {

}

std::vector<uint8_t> ChunkSerializer::decompress(const std::vector<uint8_t>& compressedData, size_t size) {

}