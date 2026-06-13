#pragma once
#define CHUNK_H

#include "blockmanager.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <atomic>
#include "chunkserializer.h"

const int CHUNK_WIDTH = 16;
const int CHUNK_HEIGHT = 256;
const int CHUNK_DEPTH = 16;

class ChunkManager;

class Chunk {
    public:
        int gridX, gridZ;
        bool isVisible = false;

        Chunk(int gridX, int gridZ, BlockManager* manager);
        ~Chunk();

        void render();
        void setLight(int localX, int localY, int localZ, uint8_t level);
        int getLight(int localX, int localY, int localZ);
        void setBlock(int x, int y, int z, uint8_t id);
        uint8_t getBlock(int x, int y, int z) const;

        bool isLoaded();
        void load();
        void setup();
        bool isSetup();

        void rebuildMesh();
        void unload();

        void setWorld(ChunkManager* world);

        void uploadToGPU();

        void beginJob() { m_activeJobs++; }
        void endJob()   { m_activeJobs--; }
        bool hasActiveJob() const { return m_activeJobs > 0; }

        const uint8_t* getBlockData() const;
        void setBlockData(const uint8_t* data, size_t size);

        int getX();
        int getY();
        int getZ();

    private:
        bool m_isReadyToRender = false;

        std::vector<float> meshVertices;

        bool m_isSetup = false;
        uint8_t* blocks;

        unsigned int VAO, VBO;
        int vertexCount;
        bool isDirty;

        uint8_t* lightMap;

        ChunkManager* m_world = nullptr;

        std::atomic<int> m_activeJobs{0};

        // private pointer to hold manager
        BlockManager* manager;

        int getIndex(int x, int y, int z) const;
        void generateMesh();
        void addBlockVertices(std::vector<float>& vertices, int localX, int localY, int localZ);
        int generateAmbientOcclusion(int localX, int localY, int localZ, enum faceDirection faceDi, int vertexID);
        void sunlightLevel();
    };
