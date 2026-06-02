#include "chunkmanager.h"
#include "chunk.h"

ChunkManager::ChunkManager() {
    for (int x = -5; x <= 5; x++) {
        for (int z = -5; z <= 5; z++) {
            glm::ivec2 pos(x, z);
            
            // 1. Allocate the clean memory shells inside your map
            m_activeChunks[pos] = new Chunk(x, z, &m_blockManager);
            
            // 2. Queue them up for the pipeline stages to process step-by-step
            m_loadList.push_back(pos);
            m_setupList.push_back(pos);
            m_rebuildList.push_back(pos);
        }
    }
}

ChunkManager::~ChunkManager() {
    for (auto const& pair : m_activeChunks) {
        delete pair.second; // Safely releases the Chunk* memory allocation
    }
    m_activeChunks.clear();
}

void ChunkManager::update(Camera Camera) {
    updateLoadList();
    updateSetupList();
    updateRebuildList();
    updateUnloadList();

    updateVisibilityList(Camera);
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

                m_flagsList.push_back(pChunk);

                Chunk* chunkXMinus = getChunk(chunkPos.x - 1, chunkPos.y);
                Chunk* chunkXPlus = getChunk(chunkPos.x + 1, chunkPos.y);
                Chunk* chunkZMinus = getChunk(chunkPos.x, chunkPos.y - 1);
                Chunk* chunkZPlus = getChunk(chunkPos.x, chunkPos.y + 1);

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
    if (m_cameraPos == Camera.Position && m_cameraView == Camera.Front && forceVisibilityUpdate == false) {
        return;
    }

    m_visibilityList.clear();
    forceVisibilityUpdate = false; 

    glm::vec3 lookDir = glm::normalize(Camera.Front);

    for (auto const& pair : m_activeChunks) {
        glm::ivec2 chunkPos = pair.first;
        Chunk* pChunk = pair.second;

        if (pChunk == nullptr || !pChunk->isLoaded()) {
            if (pChunk) pChunk->isVisible = false;
            continue;
        }

        // Calculate chunk center world coordinates
        float chunkCenterX = (pChunk->gridX * CHUNK_WIDTH) + (CHUNK_WIDTH / 2.0f);
        float chunkCenterZ = (pChunk->gridZ * CHUNK_DEPTH) + (CHUNK_DEPTH / 2.0f);

        // Compute displacement vector from player camera to chunk center
        glm::vec2 flatVector(
            chunkCenterX - m_cameraPos.x,
            chunkCenterZ - m_cameraPos.z
        );
        float distance = glm::length(flatVector);

        if (distance > MAX_RENDER_DISTANCE) {
            pChunk->isVisible = false;
            continue;
        }

        // Normalize your target vector to isolate pure angular direction
        glm::vec3 targetDir = glm::normalize(glm::vec3(flatVector.x, 0.0f, flatVector.y));

        // Flatten the look direction too so Y doesn't affect the comparison
        glm::vec3 flatLookDir = glm::normalize(glm::vec3(lookDir.x, 0.0f, lookDir.z));

        // The dot product of two normalized vectors yields cos(theta)
        float dotProduct = glm::dot(flatLookDir, targetDir);

        // Chunks very close to the camera are always visible (player may be inside them).
        // Beyond that, cull anything behind the camera with a small margin.
        bool nearby = distance < (float)(CHUNK_WIDTH * 2);
        if (!nearby && dotProduct < -0.3f) {
            pChunk->isVisible = false;
            continue;
        }

        // Passed both checks!
        pChunk->isVisible = true;
        m_visibilityList.push_back(chunkPos);
    }
}

void ChunkManager::updateRenderList() {
    m_renderList.clear();

    std::vector<std::pair<float, Chunk*>> distancePass;

    for (const glm::ivec2& chunkPos : m_visibilityList) {
        Chunk* pChunk = m_activeChunks[chunkPos];

        if (pChunk == nullptr || !pChunk->isLoaded()) {
            continue;
        }

        float chunkCenterX = (pChunk->gridX * CHUNK_WIDTH) + (CHUNK_WIDTH / 2.0f);
        float chunkCenterZ = (pChunk->gridZ * CHUNK_DEPTH) + (CHUNK_DEPTH / 2.0f);

        float dx = chunkCenterX - m_cameraPos.x;
        float dz = chunkCenterZ - m_cameraPos.z;

        float squaredDistance = (dx * dx) + (dz * dz);

        // Store the calculation into our temporary pass list
        distancePass.push_back({squaredDistance, pChunk});
    }

    // Sort the temporary list using a C++ Lambda predicate (Front-to-Back order)
    std::sort(distancePass.begin(), distancePass.end(), 
        [](const std::pair<float, Chunk*>& a, const std::pair<float, Chunk*>& b) {
            return a.first < b.first; // Change to '>' if you are sorting transparent meshes Back-to-Front later
        }
    );

    // Extract the sorted chunk pointers back into your master m_renderList array container
    for (const auto& pair : distancePass) {
        m_renderList.push_back(pair.second);
    }
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

const std::vector<Chunk*>& ChunkManager::getRenderList() const { 
    return m_renderList; 
}