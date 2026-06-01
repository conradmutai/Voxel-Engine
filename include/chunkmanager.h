#pragma once
#define CHUNKMANAGER_H

#include "camera.h"
#include "chunk.h"

#include <unordered_map>
#include <glm/glm.hpp>

// A custom hash function to allow glm::ivec2 (or a custom struct) to be used as a Key
struct KeyHash {
    std::size_t operator()(const glm::ivec2& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
    }
};

class ChunkManager {
    public: 
        // constructor and destructor
        ChunkManager();
        ~ChunkManager();

        void update(const glm::vec3& cameraPos); // takes in the address of the current camera position to carry out tasks like rendering based on its position

        // all the updates that store, load, update the chunks
        void updateLoadList();
        void updateSetupList();
        void updateRebuildList();
        void updateReloadList();
        void updateVisibilityList();
        void updateRenderList(); 

    private:
        // this is the active map that holds the current chunks
        std::unordered_map<glm::ivec2, Chunk*, KeyHash> m_activeChunks;

        // these hold the chunks which need to be loaded or rebuilt based on if it is modified in game
        std::vector<glm::ivec2> m_loadList;
        std::vector<glm::ivec2> m_rebuildList;
};
