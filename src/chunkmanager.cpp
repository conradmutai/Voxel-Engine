#include "chunkmanager.h"
#include "chunk.h"

#include <filesystem>
#include <algorithm>
#include <cmath>
#include <chunkserializer.h>


// World starts empty; chunks stream in around the player each frame
ChunkManager::ChunkManager() {
    std::filesystem::create_directories("saves/chunks/");
}

// chunk destructor
ChunkManager::~ChunkManager() {
    for (auto const& pair : m_activeChunks) {
        delete pair.second; // Safely releases the Chunk* memory allocation
    }
    m_activeChunks.clear();
}

// updates these various lists
void ChunkManager::update(Camera Camera) {
    uploadCompletedChunks();
    updateChunkStreaming(Camera);
    updateLoadList();
    updateUnloadList();

    updateVisibilityList(Camera);
    updateRenderList();

    m_cameraPos = Camera.Position;
    m_cameraView = Camera.Front;
}

void ChunkManager::updateChunkStreaming(Camera camera) {
    int playerChunkX = (int)std::floor(camera.Position.x / CHUNK_WIDTH);
    int playerChunkZ = (int)std::floor(camera.Position.z / CHUNK_DEPTH);

    // Spawn chunks within a circular render distance
    for (int dx = -RENDER_DISTANCE_CHUNKS; dx <= RENDER_DISTANCE_CHUNKS; dx++) {
        for (int dz = -RENDER_DISTANCE_CHUNKS; dz <= RENDER_DISTANCE_CHUNKS; dz++) {
            if (dx*dx + dz*dz > RENDER_DISTANCE_CHUNKS * RENDER_DISTANCE_CHUNKS) continue;

            glm::ivec2 pos(playerChunkX + dx, playerChunkZ + dz);
            if (m_activeChunks.count(pos)) continue;

            Chunk* c = new Chunk(pos.x, pos.y, &m_blockManager);
            c->setWorld(this);
            m_activeChunks[pos] = c;
            m_loadList.push_back(pos);
            m_setupList.push_back(pos);
            m_rebuildList.push_back(pos);
            forceVisibilityUpdate = true;
        }
    }

    // Unload chunks beyond the unload radius
    for (auto it = m_activeChunks.begin(); it != m_activeChunks.end(); ) {
        int dx = it->first.x - playerChunkX;
        int dz = it->first.y - playerChunkZ;

        if (dx*dx + dz*dz > UNLOAD_DISTANCE_CHUNKS * UNLOAD_DISTANCE_CHUNKS) {
            Chunk* chunk = it->second;
            if (chunk->hasActiveJob()) { ++it; continue; }

            glm::ivec2 key = it->first;
            auto removeKey = [&key](std::vector<glm::ivec2>& list) {
                list.erase(std::remove(list.begin(), list.end(), key), list.end());
            };
            removeKey(m_loadList);
            removeKey(m_setupList);
            removeKey(m_rebuildList);
            m_flagsList.erase(std::remove(m_flagsList.begin(), m_flagsList.end(), chunk), m_flagsList.end());

            if (chunk->isLoaded()) {
                m_serializer.save(chunk, it->first);
            }
            delete chunk;
            it = m_activeChunks.erase(it);
            forceVisibilityUpdate = true;
        } else {
            ++it;
        }
    }
}

void ChunkManager::updateLoadList() {
    int numOfChunksLoaded = 0;
    auto iterator = m_loadList.begin();
    while (iterator != m_loadList.end() && numOfChunksLoaded != NUM_OF_CHUNKS_LOADED_PER_FRAME) {
        Chunk* chunk = m_activeChunks[*iterator];
        if (!chunk->isLoaded()) {
            // queues the chunk loading into the thread pool reducing stress on the CPU
            glm::ivec2 pos = *iterator;

            chunk->beginJob();
            m_threadPool.enqueue([this, chunk, pos]() {
                if (!m_serializer.load(chunk, pos)) {
                    chunk->load();
                }
                chunk->setup();
                chunk->rebuildMesh();
                std::unique_lock<std::mutex> qlock(m_completedMutex);
                m_completedChunks.emplace(chunk);
            });
            numOfChunksLoaded++;
            forceVisibilityUpdate = true;
        }
        iterator = m_loadList.erase(iterator); // processed -> drop it, advance
    }
    // leftover chunks stay in m_loadList for next frame
}

void ChunkManager::updateSetupList() {
    auto iterator = m_setupList.begin();
    while (iterator != m_setupList.end()) {
        Chunk* pChunk = m_activeChunks[*iterator];
        if (pChunk->isLoaded() && !pChunk->isSetup()) {
            pChunk->setup();
            forceVisibilityUpdate = true;
            iterator = m_setupList.erase(iterator);
        } else if (pChunk->isSetup()) {
            iterator = m_setupList.erase(iterator); // already done
        } else {
            ++iterator; // not loaded yet, keep for later
        }
    }
}

// updates the rebuild list on if the chunk is updated (player has placed or broken a block)
void ChunkManager::updateRebuildList() {
    // keeps track of the num of rebuilt chunks and initializes the iterator
    int numOfRebuiltChunks = 0;
    auto iterator = m_rebuildList.begin();

    // iterates through all the chunks and inputs them into the rebuild list if it is in frame and is not greater than the max num of chunks to load per each frame
    while (iterator != m_rebuildList.end() && numOfRebuiltChunks != NUM_OF_CHUNKS_LOADED_PER_FRAME) {
        Chunk* pChunk = m_activeChunks[*iterator];
        if (pChunk->isLoaded() && pChunk->isSetup()) {
            pChunk->beginJob();
            m_threadPool.enqueue([this, pChunk] {
                pChunk->rebuildMesh();
                std::unique_lock<std::mutex> lock(m_completedMutex);
                m_completedChunks.emplace(pChunk);
            });
            m_flagsList.push_back(pChunk);
            
            glm::ivec2 chunkPos = *iterator;
            Chunk* chunkXMinus = getChunk(chunkPos.x - 1, chunkPos.y);
            Chunk* chunkXPlus  = getChunk(chunkPos.x + 1, chunkPos.y);
            Chunk* chunkZMinus = getChunk(chunkPos.x, chunkPos.y - 1);
            Chunk* chunkZPlus  = getChunk(chunkPos.x, chunkPos.y + 1);
            if (chunkXMinus) m_flagsList.push_back(chunkXMinus);
            if (chunkXPlus)  m_flagsList.push_back(chunkXPlus);
            if (chunkZMinus) m_flagsList.push_back(chunkZMinus);
            if (chunkZPlus)  m_flagsList.push_back(chunkZPlus);

            numOfRebuiltChunks++;
            forceVisibilityUpdate = true;
            iterator = m_rebuildList.erase(iterator);
        } else {
            ++iterator; // not ready this frame, keep it
        }
    }
}

// updates the unload list meaning that it informs the engine on what chunks to offload
void ChunkManager::updateUnloadList() {
    std::vector<glm::ivec2>::iterator iterator;
    
    // iterates through the unload loist 
    for (iterator = m_unloadList.begin(); iterator != m_unloadList.end(); ++iterator) {
        // grabs the chunk position (x,y) coords
        glm::ivec2 chunkPos = *iterator;
        Chunk* pChunk = m_activeChunks[chunkPos];

        // if the chunk is loaded then we unload it
        if (pChunk -> isLoaded()) {
            pChunk -> unload();
            forceVisibilityUpdate = true;
        }
    }

    // clear the list once this is done
    m_unloadList.clear();
}

// updates the visibility list to allow the chunk to load based if it is present
void ChunkManager::updateVisibilityList(Camera Camera) {
    // if the camera position is equal to the actual camera positionand the camera view is facing the front camera then return
    if (m_cameraPos == Camera.Position && m_cameraView == Camera.Front && forceVisibilityUpdate == false) {
        return;
    }

    m_visibilityList.clear();
    forceVisibilityUpdate = false; 

    glm::vec3 lookDir = glm::normalize(Camera.Front);
    std::lock_guard<std::mutex> lock(m_chunksMutex);

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
    std::lock_guard<std::mutex> lock(m_chunksMutex);

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
    std::lock_guard<std::mutex> lock(m_chunksMutex);

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

// floor division so negative coords land in the right chunk
static int floorDiv(int a, int b) {
    int q = a / b, r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) q--;
    return q;
}

uint8_t ChunkManager::getBlockWorld(int wx, int wy, int wz) {
    if (wy < 0 || wy >= CHUNK_HEIGHT) return AIR;
    int gx = floorDiv(wx, CHUNK_WIDTH);
    int gz = floorDiv(wz, CHUNK_DEPTH);
    Chunk* c = getChunk(gx, gz);
    if (!c || !c->isLoaded()) return AIR;   // neighbour not generated yet
    return c->getBlock(wx - gx * CHUNK_WIDTH, wy, wz - gz * CHUNK_DEPTH);
}

int ChunkManager::getLightWorld(int wx, int wy, int wz) {
    if (wy < 0 || wy >= CHUNK_HEIGHT) return 15;
    int gx = floorDiv(wx, CHUNK_WIDTH);
    int gz = floorDiv(wz, CHUNK_DEPTH);
    Chunk* c = getChunk(gx, gz);
    if (!c || !c->isLoaded()) return 15;
    return c->getLight(wx - gx * CHUNK_WIDTH, wy, wz - gz * CHUNK_DEPTH);
}

const std::vector<Chunk*>& ChunkManager::getRenderList() const { 
    return m_renderList; 
}

// function made to lock the completed mutex and then upload the chunks to the main thread to render
void ChunkManager::uploadCompletedChunks() {
    std::unique_lock<std::mutex> lock(m_completedMutex);
    while (!m_completedChunks.empty()) {
        Chunk* chunk = m_completedChunks.front();
        m_completedChunks.pop();
        lock.unlock();
        chunk->uploadToGPU();
        chunk->endJob();
        lock.lock();
    }
}