#pragma once
#define CHUNKMANAGER_H

#include "camera.h"
#include "chunk.h"

#include <glad/glad.h>
#include <unordered_map>
#include <glm/glm.hpp>

// A custom hash function to allow glm::ivec2 (or a custom struct) to be used as a Key
struct KeyHash {
    std::size_t operator()(const glm::ivec2& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
    }
};

const int NUM_OF_CHUNKS_LOADED_PER_FRAME = 200;
const int MAX_RENDER_DISTANCE = 1000;

class ChunkManager {
    public: 
        // constructor and destructor
        ChunkManager();
        ~ChunkManager();

        void update(Camera Camera); // takes in the address of the current camera position to carry out tasks like rendering based on its position

        // all the updates that store, load, update the chunks
        void updateLoadList();
        void updateSetupList();
        void updateRebuildList();
        void updateReloadList();
        void updateUnloadList();
        void updateVisibilityList(Camera Camera);
        void updateRenderList(); 

        const std::vector<Chunk*>& getRenderList() const;

    private:
        // master world tracking structure
        std::unordered_map<glm::ivec2, Chunk*, KeyHash> m_activeChunks;

        BlockManager m_blockManager;
        bool forceVisibilityUpdate = false;

        // these hold the chunks which need to be loaded or rebuilt based on if it is modified in game
        std::vector<glm::ivec2> m_loadList;
        std::vector<glm::ivec2> m_setupList;
        std::vector<glm::ivec2> m_rebuildList;
        std::vector<glm::ivec2> m_unloadList;
        std::vector<glm::ivec2> m_visibilityList;

        // a array that stores the chunks in the render list
        std::vector<Chunk*> m_renderList;

        // stores the flag updates for the chunks
        std::vector<Chunk*> m_flagsList;

        // holds a static camera position thats updated everytime its called
        glm::vec3 m_cameraPos;
        glm::vec3 m_cameraView;

        // helper functions
        Chunk* getChunk(int gridX, int gridZ);
};
