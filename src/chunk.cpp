#include "chunk.h"
#include "chunkmanager.h"
#include <glm/gtc/noise.hpp>

static uint8_t sunlightLookup[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static uint8_t artificialLightLookup[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

Chunk::Chunk(int gridX, int gridZ, BlockManager* manager) : gridX(gridX), gridZ(gridZ), manager(manager), isDirty(true), vertexCount(0), isVisible(true) {
    // 1. Heap Allocation for the 1D Array (Initialized to 0/AIR)
    blocks = nullptr;
    lightMap = nullptr;

    // // 2. Generate Basic Flat Terrain 
    // for (int z = 0; z < CHUNK_DEPTH; ++z) {
    //     for (int y = 0; y < 16; ++y) {
    //         for (int x = 0; x < CHUNK_WIDTH; ++x) {
    //             if(y == 15) {
    //                 setBlock(x, y, z, GRASS);
    //             } else if (y < 15 && y > 11) {
    //                 setBlock(x, y, z, DIRT);
    //             }else {
    //                 setBlock(x, y, z, STONE);
    //             }
    //         }
    //     }
    // }


    // ---------------------- AO TESTING CODE ----------------------
    // for (int x = 0; x < CHUNK_WIDTH; x++) {
    //     setBlock(x, 16, 3, STONE);
    // }
    // -------------------------------------------------------------

    // generates the buffer and vertex arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

Chunk::~Chunk() {
    delete[] blocks;
    delete[] lightMap;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Chunk::load() {
    blocks = new uint8_t[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH]();
    lightMap = new uint8_t[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH]();

    // declaring variables which hold the global address value (wX is worldX, wZ is worldZ)
    int wX, wY, wZ;
    float scaleX, scaleZ;
    float noiseVal, terrainHeight, baseHeight = 64.0f, heightScale = 32.0f;


    // 2. Generate Terrain (NEW MODEL FOR PHASE 5)
    for (int z = 0; z < CHUNK_DEPTH; ++z) {
        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            // Calculates the global coordinates
            wX = (gridX * CHUNK_WIDTH) + x;
            wZ = (gridZ * CHUNK_DEPTH) + z;

            // Scales coordinates for the noise frequency
            scaleX = wX * 0.01f;
            scaleZ = wZ * 0.01f;

            // Generates the noise value from glm::noise in range (-1.0 to 1.0) 
            noiseVal = glm::perlin(glm::vec2(scaleX, scaleZ));

            // Generates the value for the terrain height at each coord
            terrainHeight = baseHeight + (noiseVal * heightScale);

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                wY = y;

                if (y < (int) terrainHeight) {
                    if (y == (int) terrainHeight - 1) { 
                        setBlock(x, y, z, GRASS);
                    } else {
                        if (y > (int) terrainHeight - 10) {
                            setBlock(x, y, z, DIRT);
                        } else {
                            setBlock(x, y, z, STONE); 
                        }
                    }
                } else {
                    setBlock (x, y, z, AIR);
                }
            }
        }
    }
}

bool Chunk::isLoaded() {
    if (blocks != NULL && lightMap != NULL) {
        return true;
    }

    return false;
}

void Chunk::setup() {
    sunlightLevel();

    m_isSetup = true; 
    isDirty = true;
}

bool Chunk::isSetup() {
    if (m_isSetup) {
        return true;
    }

    return false;
}

void Chunk::rebuildMesh() {
    generateMesh();
}

void Chunk::unload() {
    delete[] blocks;
    delete[] lightMap;

    blocks = nullptr;
    lightMap = nullptr;
}

void Chunk::setWorld(ChunkManager* world) {
    m_world = world;
}

int Chunk::getIndex(int x, int y, int z) const {
    // Executes the flattening formula 
    int index = x + (y * CHUNK_WIDTH) + (z * CHUNK_WIDTH * CHUNK_HEIGHT); 
    return index;
}

void Chunk::setBlock(int x, int y, int z, uint8_t id) {
    // sets the block ids, and sets isDirty to true to show that the chunk is not clean
    if (x < CHUNK_WIDTH && y < CHUNK_HEIGHT && z < CHUNK_DEPTH) {
        blocks[getIndex(x,y,z)] = id;
        isDirty = true;
    }
}

uint8_t Chunk::getBlock(int x, int y, int z) const {
    if (y < 0 || y >= CHUNK_HEIGHT) return AIR;
    if (x >= 0 && x < CHUNK_WIDTH && z >= 0 && z < CHUNK_DEPTH) {
        return blocks ? blocks[getIndex(x,y,z)] : AIR;
    }
    if (m_world) {
        return m_world->getBlockWorld(gridX * CHUNK_WIDTH + x, y, gridZ * CHUNK_DEPTH + z);
    }
    return AIR;
}

// sets the light level for the block
void Chunk::setLight(int localX, int localY, int localZ, uint8_t level) {
    int index = 0;
    
    if (localX < CHUNK_WIDTH && localY < CHUNK_HEIGHT && localZ < CHUNK_DEPTH) {
        index = getIndex(localX, localY, localZ);
        lightMap[index] = level;
        isDirty = true;
    }
}

// gets the light level from the lightMap for the block
int Chunk::getLight(int localX, int localY, int localZ) {
    if (localX >= 0 && localX < CHUNK_WIDTH && localY >= 0 && localY < CHUNK_HEIGHT && localZ >= 0 && localZ < CHUNK_DEPTH) {
        return lightMap[getIndex(localX, localY, localZ)];
    }
    if (m_world) {
        return m_world->getLightWorld(gridX * CHUNK_WIDTH + localX, localY, gridZ * CHUNK_DEPTH + localZ);
    }
    return 15;
}

// sets the sunlight level for all the blocks at initialization
void Chunk::sunlightLevel() {
    bool isSkyLightBlocked = false;
    int yCoord = CHUNK_HEIGHT - 1;

    for (int xCoord = 0; xCoord < CHUNK_WIDTH; xCoord++) {
        for (int zCoord = 0; zCoord < CHUNK_DEPTH; zCoord++) {
            isSkyLightBlocked = false;
            yCoord = CHUNK_HEIGHT - 1;

            while(yCoord >= 0) {
                if(getBlock(xCoord, yCoord, zCoord) != AIR && getBlock(xCoord, yCoord, zCoord) != GLASS) {
                    isSkyLightBlocked = true;
                }
                if (!isSkyLightBlocked) 
                    setLight(xCoord, yCoord, zCoord, 15);
                yCoord--;
            }
        }
    }
}

void Chunk::render() {
    // checks to see if we are dealing with an air block
     if (m_isReadyToRender && vertexCount > 0) {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }
}

void Chunk::generateMesh() {
    // creates a a mesh vertices
    meshVertices.clear();
    
    // iterates over all ways of depth, height, width (in order) to add block to the vertex
    for (int z = 0; z < CHUNK_DEPTH; z++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                if (getBlock(x, y, z) != AIR) {
                    addBlockVertices(meshVertices, x, y, z);
                }
            }
        }
    }

    isDirty = false;
}

// uploads a chunk to the GPU
void Chunk::uploadToGPU() {
    vertexCount = (int)(meshVertices.size() / 7);

    if (vertexCount == 0) return;

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(3);

    m_isReadyToRender = true;
}

int Chunk::generateAmbientOcclusion(int localX, int localY, int localZ, enum faceDirection faceDir, int vertexID) {
    // 1. Rename the tracking variables to be axis-agnostic and clean
    int axisAX = localX, axisAY = localY, axisAZ = localZ;
    int axisBX = localX, axisBY = localY, axisBZ = localZ;
    int diagX   = localX, diagY   = localY, diagZ   = localZ;

    if (faceDir == FACE_TOP) {
        int topFace = localY + 1;
        axisAY = topFace; axisBY = topFace; diagY = topFace;

        if (vertexID == 0) { // Bottom-Left
            axisAX = localX - 1; axisBZ = localZ + 1;
            diagX  = localX - 1; diagZ  = localZ + 1;
        }
        else if (vertexID == 1) { // Bottom-Right
            axisAX = localX + 1; axisBZ = localZ + 1;
            diagX  = localX + 1; diagZ  = localZ + 1;
        }
        else if (vertexID == 2) { // Top-Right
            axisAX = localX + 1; axisBZ = localZ - 1;
            diagX  = localX + 1; diagZ  = localZ - 1;
        }
        else if (vertexID == 3) { // Top-Left
            axisAX = localX - 1; axisBZ = localZ - 1;
            diagX  = localX - 1; diagZ  = localZ - 1;
        }
    } 
    else if (faceDir == FACE_BOTTOM) {
        int bottomFace = localY - 1; // Fixed: altered from localZ - 1
        axisAY = bottomFace; axisBY = bottomFace; diagY = bottomFace;

        if (vertexID == 0) { 
            axisAX = localX - 1; axisBZ = localZ - 1;
            diagX  = localX - 1; diagZ  = localZ - 1;
        }
        else if (vertexID == 1) { 
            axisAX = localX + 1; axisBZ = localZ - 1;
            diagX  = localX + 1; diagZ  = localZ - 1;
        }
        else if (vertexID == 2) { 
            axisAX = localX + 1; axisBZ = localZ + 1;
            diagX  = localX + 1; diagZ  = localZ + 1;
        }
        else if (vertexID == 3) { 
            axisAX = localX - 1; axisBZ = localZ + 1;
            diagX  = localX - 1; diagZ  = localZ + 1;
        }
    }
    else if (faceDir == FACE_FRONT) {
        int frontFace = localZ + 1;
        axisAZ = frontFace; axisBZ = frontFace; diagZ = frontFace;

        if (vertexID == 0) {
            axisAX = localX - 1; axisBY = localY - 1;
            diagX = localX - 1; diagY = localY - 1;
        } 
        else if (vertexID == 1) {
            axisAX = localX + 1; axisBY = localY - 1;
            diagX = localX + 1; diagY = localY - 1;
        } 
        else if (vertexID == 2) {
            axisAX = localX + 1; axisBY = localY + 1;
            diagX = localX + 1; diagY = localY + 1;
        }
        else if (vertexID == 3) {
            axisAX = localX - 1; axisBY = localY + 1;
            diagX = localX - 1; diagY = localY + 1;
        }
    }
    else if (faceDir == FACE_BACK) {
        int backFace = localZ - 1;
        axisAZ = backFace; axisBZ = backFace; diagZ = backFace;

        if (vertexID == 0) {
            axisAX = localX + 1; axisBY = localY - 1;
            diagX = localX + 1; diagY = localY - 1;
        }
        else if (vertexID == 1) {
            axisAX = localX - 1; axisBY = localY - 1;
            diagX = localX - 1; diagY = localY - 1;
        }
        else if (vertexID == 2) {
            axisAX = localX - 1; axisBY = localY + 1;
            diagX = localX - 1; diagY = localY + 1;
        }
        else if (vertexID == 3) {
            axisAX = localX + 1; axisBY = localY + 1;
            diagX = localX + 1; diagY = localY + 1; // Fixed duplicate line copy-paste bug
        }
    }
    else if (faceDir == FACE_RIGHT) {
        int rightFace = localX + 1;
        axisAX = rightFace; axisBX = rightFace; diagX = rightFace;

        if (vertexID == 0) {
            axisAZ = localZ - 1; axisBY = localY - 1;
            diagZ = localZ - 1;  diagY = localY - 1; 
        }
        else if (vertexID == 1) {
            axisAZ = localZ + 1; axisBY = localY - 1;
            diagZ = localZ + 1;  diagY = localY - 1;
        }
        else if (vertexID == 2) {
            axisAZ = localZ + 1; axisBY = localY + 1;
            diagZ = localZ + 1;  diagY = localY + 1;
        }
        else if (vertexID == 3) {
            axisAZ = localZ - 1; axisBY = localY + 1;
            diagZ = localZ - 1;  diagY = localY + 1;
        }
    }
    else if (faceDir == FACE_LEFT) {
        int leftFace = localX - 1;
        axisAX = leftFace; axisBX = leftFace; diagX = leftFace;

        if (vertexID == 0) {
            axisAZ = localZ - 1; axisBY = localY - 1;
            diagZ = localZ - 1;  diagY = localY - 1;
        }
        else if (vertexID == 1) {
            axisAZ = localZ + 1; axisBY = localY - 1;
            diagZ = localZ + 1;  diagY = localY - 1;
        }
        else if (vertexID == 2) {
            axisAZ = localZ + 1; axisBY = localY + 1;
            diagZ = localZ + 1;  diagY = localY + 1;
        }
        else if (vertexID == 3) {
            axisAZ = localZ - 1; axisBY = localY + 1;
            diagZ = localZ - 1;  diagY = localY + 1;
        }
    }

    // 2. Read the blocks using the updated, readable terminology
    uint8_t blockAxisA = getBlock(axisAX, axisAY, axisAZ);
    uint8_t blockAxisB = getBlock(axisBX, axisBY, axisBZ);
    uint8_t blockDiag  = getBlock(diagX,  diagY,  diagZ);

    bool isAxisASolid = (blockAxisA != AIR && blockAxisA != GLASS);
    bool isAxisBSolid = (blockAxisB != AIR && blockAxisB != GLASS);
    bool isDiagSolid  = (blockDiag  != AIR && blockDiag  != GLASS);

    int solidNeighbors = 0;
    if (isAxisASolid) solidNeighbors++;
    if (isAxisBSolid) solidNeighbors++;
    if (isDiagSolid)  solidNeighbors++;

    // Edge-case handling: If both sides are closed, force total occlusion corner shadow
    if (isAxisASolid && isAxisBSolid) {
        return 0; 
    }

    if (solidNeighbors == 0) return 3;
    if (solidNeighbors == 1) return 2;
    if (solidNeighbors == 2) return 1;
    return 0;
}

void Chunk::addBlockVertices(std::vector<float>& meshVertices, int localX, int localY, int localZ) {
    // 1. Gets block id and checks if its is air
    uint8_t blockID = getBlock(localX, localY, localZ);
    if (blockID == AIR) return;
    
    // 2. Convert local chunk coordinates to world coordinates
    float wX = (gridX * CHUNK_WIDTH) + localX;
    float wY = localY;
    float wZ = (gridZ * CHUNK_DEPTH) + localZ;

    std::vector<float> uvs;
    float startU, startV, endU, endV;

    // checks above the current block and creates a face with texture mapped
    if (localY == CHUNK_HEIGHT - 1 || manager->isTransparent(getBlock(localX, localY + 1, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_TOP);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        // gets the light level of the block above
        float lightVal = (float) getLight(localX, localY + 1, localZ); // gets the light level for the block above

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_TOP, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_TOP, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_TOP, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_TOP, 3); // Top-Left Corner

        // compares the two diagonals and if ao0 + ao2 is less than ao1 + ao3 then the quad is split down the diagonal
        if (ao0 + ao2 < ao1 + ao3) {
            // reordered the faces to perform Anistropy
            float face[] = {
                // X, Y, Z                                // U,   V,      AO,          light
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   startV, (float) ao1, lightVal,  // Bottom-Right
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   (float) ao2, lightVal,  // Top-Right
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,   (float) ao3, lightVal,  // Top-Left
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,   (float) ao3, lightVal,  // Top-Left
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, startV, (float) ao0, lightVal,  // Bottom-Left
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   startV, (float) ao1, lightVal,  // Bottom-Right
            };

            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        } else {
            float face[] = {
                // X, Y, Z                                // U,   V,      AO,          light
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, startV, (float) ao0, lightVal, // Bottom-Left
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   startV, (float) ao1, lightVal, // Bottom-Right
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   (float) ao2, lightVal, // Top-Right
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   (float) ao2, lightVal, // Top-Right
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,   (float) ao3, lightVal, // Top-Left
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, startV, (float) ao0, lightVal // Bottom-Left
            };

            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        }
    }
    // checks below the block and creates a face
    if (localY == 0 || manager->isTransparent(getBlock(localX, localY - 1, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_BOTTOM);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        // gets the light level of the block below for generation
        float lightVal = (float) getLight(localX, localY - 1, localZ);

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_BOTTOM, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_BOTTOM, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_BOTTOM, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_BOTTOM, 3); // Top-Left Corner

        if (ao0 + ao2 < ao1 + ao3) {
            float face[] = {
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  endU,   startV, (float) ao1, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        } else {
            float face[] = {
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        }
    }
    // checks right of the block and creates a face and maps the textures
    if (localX == CHUNK_WIDTH - 1 || manager->isTransparent(getBlock(localX + 1, localY, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_RIGHT);

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_RIGHT, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_RIGHT, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_RIGHT, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_RIGHT, 3); // Top-Left Corner

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        // gets the light level of the block to the right of the block
        float lightVal = (float) getLight(localX + 1, localY, localZ);

        if (ao0 + ao2 < ao1 + ao3) {
            float face[] = {
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        } else {
            float face[] = {
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        }
    }
    // checks left of the block and creates a face and maps the textures
    if (localX == 0 || manager->isTransparent(getBlock(localX - 1, localY, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_LEFT);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_LEFT, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_LEFT, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_LEFT, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_LEFT, 3); // Top-Left Corner

        // gets the light level of the block to the left 
        float lightVal = (float) getLight(localX - 1, localY, localZ);

        if (ao0 + ao2 < ao1 + ao3) {
            float face[] = {
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        } else {
            float face[] = {
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        }
    }
    // checks front of the block and creates a face and maps the textures
    if (localZ == CHUNK_DEPTH - 1 || manager->isTransparent(getBlock(localX, localY, localZ + 1))) {
        uvs = manager->uvCalculator(blockID, FACE_FRONT);

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_FRONT, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_FRONT, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_FRONT, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_FRONT, 3); // Top-Left Corner

        // gets the light level fo the block in fornt
        float lightVal = (float) getLight(localX, localY, localZ + 1);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];
        
        if (ao0 + ao2 < ao1 + ao3) {
            float face[] = {
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  startU, startV, (float) ao0, lightVal,
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        } else {
            float face[] = {
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  startU, startV, (float) ao0, lightVal,
                wX + 0.5f, wY - 0.5f, wZ + 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX + 0.5f, wY + 0.5f, wZ + 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX - 0.5f, wY + 0.5f, wZ + 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX - 0.5f, wY - 0.5f, wZ + 0.5f,  startU, startV, (float) ao0, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        }
    }
    // checks back of the block and creates a face and maps the textures
    if (localZ == 0 || manager->isTransparent(getBlock(localX, localY, localZ - 1))) {
        uvs = manager->uvCalculator(blockID, FACE_BACK);

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_BACK, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_BACK, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_BACK, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_BACK, 3); // Top-Left Corner

        // gets the light level of the blcok in the back
        float lightVal = (float) getLight(localX, localY, localZ - 1);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        if (ao0 + ao2 < ao1 + ao3) {
            float face[] = {
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  endU,   startV, (float) ao1, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        } else {
            float face[] = {
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
                wX - 0.5f, wY - 0.5f, wZ - 0.5f,  endU,   startV, (float) ao1, lightVal,
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX - 0.5f, wY + 0.5f, wZ - 0.5f,  endU,   endV,   (float) ao2, lightVal,
                wX + 0.5f, wY + 0.5f, wZ - 0.5f,  startU, endV,   (float) ao3, lightVal,
                wX + 0.5f, wY - 0.5f, wZ - 0.5f,  startU, startV, (float) ao0, lightVal,
            };
            meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
        }
}
}