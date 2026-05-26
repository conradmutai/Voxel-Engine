#pragma once
#define CHUNK_H

#include "blockmanager.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

const int CHUNK_WIDTH = 16;
const int CHUNK_HEIGHT = 256;
const int CHUNK_DEPTH = 16;

uint8_t sunlightLevel[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t artificialLightLevel[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

class Chunk {
    public:
        Chunk(int gridX, int gridZ, BlockManager* manager);
        ~Chunk();

        void render();
        void setLight(int localX, int localY, int localZ, uint8_t level);
        int getLight(int localX, int localY, int localZ);
        void setBlock(int x, int y, int z, uint8_t id);
        uint8_t getBlock(int x, int y, int z) const;

    private:
        int gridX, gridZ;
        uint8_t* blocks;
        
        unsigned int VAO, VBO;
        int vertexCount;
        bool isDirty;

        uint8_t* lightMap;

        // private pointer to hold manager
        BlockManager* manager;

        int getIndex(int x, int y, int z) const;
        void generateMesh();
        void addBlockVertices(std::vector<float>& vertices, int localX, int localY, int localZ);
        int generateAmbientOcclusion(int localX, int localY, int localZ, enum faceDirection faceDi, int vertexID);
        void sunlightLevel();
    };
