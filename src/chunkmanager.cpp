#include "chunkmanager.h"
#include "chunk.h"

ChunkManager::ChunkManager() {

}

void ChunkManager::update(Camera Camera) {
    updateLoadList();
    updateSetupList();
    updateRebuildList();
    updateReloadList();
    updateUnloadList();

    updateVisibilityList(Camera);

    if (m_cameraPos != Camera.Position || m_cameraView != Camera.Front) 
        updateRenderList();
        
    m_cameraPos = Camera.Position;
    m_cameraView = Camera.Front;
}

void ChunkManager::updateLoadList() {
    // checks the num of chunks loaded
    int numOfChunksLoaded = 0;

    // iterator declared to go through all the chunks in the load list
    std::vector<glm::ivec2>::iterator iterator;
    for (iterator = m_loadList.begin(); iterator != m_loadList.end() && numOfChunksLoaded != NUM_OF_CHUNKS_LOADED_PER_FRAME; ++iterator) {
        // retrieves the chunk pos from the iterator
        glm::ivec2 chunkPos = *iterator;
        
        // assigns the chunk
        Chunk* Chunk = m_activeChunks[chunkPos];

        if(Chunk -> isLoaded() == false) {
            if (numOfChunksLoaded != NUM_OF_CHUNKS_LOADED_PER_FRAME) {
                Chunk -> load();
                numOfChunksLoaded++;
                forceVisibilityUpdate = true;
            }
        }
    }

    m_loadList.clear();
}

void ChunkManager::updateSetupList() {
    // initializes the iterator
    std::vector<glm::ivec2>::iterator iterator;

    glm::ivec2 chunkPos;
    Chunk* pChunk;

    // loops through the setup list
    for (iterator = m_setupList.begin(); iterator != m_setupList.end(); ++iterator) {
        // obtains the iterator
        chunkPos = *iterator;

        // gets a chunk from that position
        pChunk = m_activeChunks[chunkPos];
        if (pChunk -> isLoaded() && pChunk -> isSetup() == false) { // checks if the chunk is loaded and if it is setup
            pChunk -> setup();
            if (pChunk -> isSetup()) 
                forceVisibilityUpdate = true;
        }
    }

    m_setupList.clear();
}

void ChunkManager::updateRebuildList() {
    int numOfRebuiltChunks = 0;
    std::vector<glm::ivec2>::iterator iterator;

    glm::ivec2 chunkPos;
    Chunk* pChunk;

    for (iterator = m_rebuildList.begin(); iterator != m_rebuildList.end(); ++iterator) {
        chunkPos = *iterator;

        // pChunk is present Chunk
        pChunk = m_activeChunks[chunkPos];
        if(pChunk -> isLoaded() && pChunk -> isSetup()) {
            if (numOfRebuiltChunks != NUM_OF_CHUNKS_LOADED_PER_FRAME) {
                pChunk -> rebuildMesh();

                // done to check neighbours
                m_flagsList.push_back(pChunk);

                // use the loop's chunkPos instead of accessing private Chunk members
                Chunk* chunkXMinus = getChunk(chunkPos.x - 1, chunkPos.y);
                Chunk* chunkXPlus = getChunk(chunkPos.x + 1, chunkPos.y);
                Chunk* chunkZMinus = getChunk(chunkPos.x, chunkPos.y - 1);
                Chunk* chunkZPlus = getChunk(chunkPos.x, chunkPos.y + 1);

                // only rebuilds a certain number of chunks per frame
                if (chunkXMinus != NULL) m_flagsList.push_back(chunkXMinus);
                if (chunkXPlus != NULL) m_flagsList.push_back(chunkXPlus);
                if (chunkZMinus != NULL) m_flagsList.push_back(chunkZMinus);
                if (chunkZPlus != NULL) m_flagsList.push_back(chunkZPlus);

                numOfRebuiltChunks++;
                forceVisibilityUpdate = true;
            }
        }
    }

    m_rebuildList.clear();
}

void ChunkManager::updateUnloadList() {
    std::vector<glm::ivec2>::iterator iterator;
    
    for (iterator = m_unloadList.begin(); iterator != m_unloadList.end(); ++iterator) {
        glm::ivec2 chunkPos = *iterator;
        Chunk* pChunk = m_activeChunks[chunkPos];

        if (pChunk -> isLoaded()) {
            pChunk -> unload();
            forceVisibilityUpdate = true;
        }
    }

    m_unloadList.clear();
}

void ChunkManager::updateVisibilityList(Camera Camera) {
    if (m_cameraPos != Camera.Position && forceVisibilityUpdate == false) {
        return;
    }

    m_visibilityList.clear();
    forceVisibilityUpdate = false; 

    glm::vec3 lookDir = glm::normalize(Camera.Front);

    for (auto const& [chunkPos, pChunk] : m_activeChunks) {
        if (pChunk == nullptr || !pChunk->isLoaded()) {
            if (pChunk) pChunk->isVisible = false;
            continue;
        }

        // Calculate chunk center world coordinates
        float chunkCenterX = (pChunk->gridX * CHUNK_WIDTH) + (CHUNK_WIDTH / 2.0f);
        float chunkCenterZ = (pChunk->gridZ * CHUNK_DEPTH) + (CHUNK_DEPTH / 2.0f);

        // Compute displacement vector from player camera to chunk center
        glm::vec3 targetVector(
            chunkCenterX - m_cameraPos.x,
            128.0f - m_cameraPos.y, // Using world mid-height baseline
            chunkCenterZ - m_cameraPos.z
        );

        float distance = glm::length(targetVector);

        if (distance > MAX_RENDER_DISTANCE) {
            pChunk->isVisible = false;
            continue;
        }

        // Normalize your target vector to isolate pure angular direction
        glm::vec3 targetDir = glm::normalize(targetVector);
        
        // The dot product of two normalized vectors yields cos(theta)
        float dotProduct = glm::dot(lookDir, targetDir);


        // If the dot product is less than cos(45), the chunk is outside your 90-degree visual cone.
        if (dotProduct < 0.707f) {
            pChunk->isVisible = false;
            continue;
        }

        // Passed both checks!
        pChunk->isVisible = true;
        m_visibilityList.push_back(chunkPos);
    }
}

void ChunkManager::updateRenderList() {
    
}

// private helper function to get chunk
Chunk* ChunkManager::getChunk(int gridX, int gridZ) {
    // Bundle the incoming integers into a 2D grid vector key
    glm::ivec2 key(gridX, gridZ);

    // Search your master map tracking container for that key
    auto search = m_activeChunks.find(key);

    // Return the pointer if found, or nullptr if it hasn't generated yet
    if (search != m_activeChunks.end()) {
        return search->second;
    }
    return nullptr;
}