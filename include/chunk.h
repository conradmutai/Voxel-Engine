#pragma once
#define CHUNK_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

const int CHUNK_WIDTH = 16;
const int CHUNK_HEIGHT = 256;
const int CHUNK_DEPTH = 16;

enum BlockType : uint8_t {
    AIR = 0,
    DIRT = 1,
    GRASS = 2,
    STONE = 3
};

class Chunk {
    public:
        Chunk(int gridX, int gridZ);
        ~Chunk();

        void render();
        void setBlock(int x, int y, int z, uint8_t id);
        uint8_t getBlock(int x, int y, int z) const;

    private:
        int gridX, gridZ;
        uint8_t* blocks;
        
        unsigned int VAO, VBO;
        int vertexCount;
        bool isDirty;

        int getIndex(int x, int y, int z) const;
        void generateMesh();
        void addBlockVertices(std::vector<float>& vertices, int localX, int localY, int localZ);
    };
