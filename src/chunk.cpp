#include "chunk.h"

Chunk::Chunk(int gridX, int gridZ, BlockManager* manager) : gridX(gridX), gridZ(gridZ), manager(manager), isDirty(true), vertexCount(0) {
    // 1. Heap Allocation for the 1D Array (Initialized to 0/AIR)
    blocks = new uint8_t[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH]();

    // 2. Generate Basic Flat Terrain 
    for (int z = 0; z < CHUNK_DEPTH; ++z) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < CHUNK_WIDTH; ++x) {
                if(y == 15) {
                    setBlock(x, y, z, GRASS);
                } else if (y < 15 && y > 11) {
                    setBlock(x, y, z, DIRT);
                }else {
                    setBlock(x, y, z, STONE);
                }
            }
        }
    }

    // ---------------------- AO TESTING CODE ----------------------
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        setBlock(x, 16, 3, STONE);
    }
    // -------------------------------------------------------------

    // generates the buffer and vertex arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

Chunk::~Chunk() {
    delete[] blocks;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
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
    // checks to see if the block coords fall into a valid range for the chunk dimensions
    if (x < CHUNK_WIDTH && y < CHUNK_HEIGHT && z < CHUNK_DEPTH) {
        return blocks[getIndex(x,y,z)];
    } 

    return AIR;
}

void Chunk::render() {
    // if the model isDirty then we jump to generateMesh()
    if(isDirty) {
        generateMesh();
    }

    // checks to see if we are dealing with an air block
    if(vertexCount > 0) {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }
}

void Chunk::generateMesh() {
    // creates a a mesh vertices
    std::vector<float> meshVertices;
    
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

    // dividides by 5 to get the actual count of the vertices
    vertexCount = meshVertices.size() / 6;

    // binds the vertex array output
    glBindVertexArray(VAO);

    // binds the buffer and the meshvertices to the vertex buffer output
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_DYNAMIC_DRAW); // changed from GL_STATIC_DRAW to GL_DYNAMIC_DRAW to provide dynamic buffer updates and performance updates

    // maps the vertex attribute array for both the points of the vertex coords and the texture coords
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    isDirty = false;
}

int Chunk::generateAmbientOcclusion(int localX, int localY, int localZ, enum faceDirection faceDir, int vertexID) {
    // 1. Rename the tracking variables to be axis-agnostic and clean
    int axisAX = localX, axisAY = localY, axisAZ = localZ;
    int axisBX = localX, axisBY = localY, axisBZ = localZ;
    int diagX   = localX, diagY   = localY, diagZ   = localZ;

    if (faceDir == FACE_TOP) {
        int topFace = localY + 1;
        axisAY = topFace; axisBY = topFace; diagY = topFace;

        if (vertexID == 0) { // Bottom-Left Corner (Near-Left)
            // Axis A moves along -X, Axis B moves along +Z
            axisAX = localX - 1; axisBZ = localZ + 1;
            // Diagonal is the intersection of both movements
            diagX  = localX - 1; diagZ  = localZ + 1;
        }
        if (vertexID == 1) { // Bottom-Right Corner (Near-Right)
            axisAX = localX + 1; axisBZ = localZ + 1;
            diagX = localX + 1; diagZ = localZ + 1;
        }
        if (vertexID == 2) { // Top-Right Corner (Far-Right)
            axisAX = localX + 1; axisBZ = localZ - 1;
            diagX = localX + 1; diagZ = localZ - 1;
        }
        if (vertexID == 3) { // Top-Left Corner (Far-Left)
            axisAX = localX - 1; axisBZ = localZ - 1;
            diagX = localX - 1; diagZ = localZ - 1;
        }
    } 
    if (faceDir == FACE_FRONT) {
        int frontFace = localZ + 1;
        axisAZ = frontFace; axisBZ = frontFace; diagZ = frontFace;

        if (vertexID == 0) {
            axisAX = localX - 1; axisBY = localY - 1;
            diagX = localX - 1; diagY = localY - 1;
        } 
        if (vertexID == 1) {
            axisAX = localX + 1; axisBY = localY - 1;
            diagX = localX + 1; diagY = localY - 1;
        } 
        if (vertexID == 2) {
            axisAX = localX + 1; axisBY = localY + 1;
            diagX = localX + 1; diagY = localY + 1;
        }
        if (vertexID == 3) {
            axisAX = localX - 1; axisBY = localY + 1;
            diagX = localX - 1; diagY = localY + 1;
        }
    }
    if (faceDir == FACE_RIGHT) {
        int rightFace = localX + 1;
        axisAX = rightFace; axisBX = rightFace; diagX = rightFace;

        if (vertexID == 0) {
            axisAZ = localZ - 1; axisBY = localY - 1;
            diagZ = localZ - 1; diagY = localY - 1; 
        }
        if (vertexID == 1) {
            axisAZ = localZ + 1; axisBY = localY - 1;
            diagZ = localZ + 1; diagY = localY - 1;
        }
        if (vertexID == 2) {
            axisAZ = localZ + 1; axisBY = localY + 1;
            diagZ = localZ + 1; diagY = localY + 1;
        }
        if (vertexID == 3) {
            axisAZ = localZ - 1; axisBY = localY + 1;
            diagZ = localZ - 1; diagY = localY + 1;
        }
    }
    if (faceDir == FACE_BOTTOM) {
        int bottomFace = localZ - 1;
        axisAY = bottomFace; axisBY = bottomFace; diagY = bottomFace;

        if (vertexID == 0) {
            axisAX = localX - 1; axisBZ = localZ - 1;
            diagX = localX - 1; diagZ = localZ - 1;
        }
        if (vertexID == 1) {
            axisAX = localX + 1; axisBZ = localZ - 1;
            diagX = localX + 1; diagZ = localZ - 1;
        }
        if (vertexID == 2) {
            axisAX = localX - 1; axisBZ = localZ - 1;
            diagX = localX - 1; diagZ = localZ - 1;
        }
        if (vertexID == 3) {
            axisAX = localX + 1; axisBZ = localZ + 1;
            diagX = localX + 1; axisBZ = localZ + 1;
        }
    }
    if (faceDir == FACE_LEFT) {
        int leftFace = localX - 1;
        axisAX = leftFace; axisBX = leftFace; diagX = leftFace;

        if (vertexID == 0) {
            axisAY = localY - 1; axisBZ = localZ - 1;
            diagY = localY - 1; diagZ = localZ - 1;
        }
        if (vertexID == 1) {
            axisAY = localY - 1; axisBZ = localZ + 1;
            diagY = localY - 1; diagZ = localZ + 1;
        }
        if (vertexID == 2) {
            axisAY = localY + 1; axisBZ = localZ + 1;
            diagY = localY + 1; diagZ = localZ + 1;
        }
        if (vertexID == 3) {
            axisAY = localY + 1; axisBZ = localZ - 1;
            diagY = localY + 1; diagZ = localZ - 1;
        }
    }
    if (faceDir == FACE_BACK) {
        int backFace = localZ - 1;
        axisAZ = backFace; axisBZ = backFace; diagZ = backFace;

        if (vertexID == 0) {
            axisAX = localX + 1; axisBY = localY - 1;
            diagX = localX + 1; diagY = localY - 1;
        }
        if (vertexID == 1) {
            axisAX = localX - 1; axisBY = localY - 1;
            diagX = localX - 1; diagY = localY - 1;
        }
        if (vertexID == 2) {
            axisAX = localX - 1; axisBY = localY + 1;
            diagX = localX - 1; diagY = localY + 1;
        }
        if (vertexID == 3) {
            axisAX = localX + 1; axisBY = localY + 1;
            diagX = localX + 1; diagX = localX + 1;
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

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_TOP, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_TOP, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_TOP, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_TOP, 3); // Top-Left Corner

        float face[] = {
            // X, Y, Z                                // U,   V,      AO
            wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, startV, (float) ao0, // Bottom-Left
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   startV, (float) ao1, // Bottom-Right
            wX + 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   (float) ao2, // Top-Right
            wX + 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   (float) ao2, // Top-Right
            wX - 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,   (float) ao3, // Top-Left
            wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, startV, (float) ao0 // Bottom-Left
        };

        meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
    }
    // checks below the block and creates a face
    if (localX == 0 || manager->isTransparent(getBlock(localX, localY - 1, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_BOTTOM);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_BOTTOM, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_BOTTOM, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_BOTTOM, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_BOTTOM, 3); // Top-Left Corner

        float face[] = {
            wX - 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV, (float) ao0,
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          endU,   startV, (float) ao1,
            wX + 0.5f, wY - 0.5f, wZ + 0.5f,          endU,   endV,   (float) ao2,
            wX + 0.5f, wY - 0.5f, wZ + 0.5f,          endU,   endV,   (float) ao2,
            wX - 0.5f, wY - 0.5f, wZ + 0.5f,          startU, endV,   (float) ao3,
            wX - 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV, (float) ao0
        };

        meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
    }
    // checks right of the block and creates a face and maps the textures
    if (localX == CHUNK_WIDTH + 1 || manager->isTransparent(getBlock(localX + 1, localY, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_RIGHT);

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_RIGHT, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_RIGHT, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_RIGHT, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_RIGHT, 3); // Top-Left Corner

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        float face[] = {
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV,  (float) ao0, // Bottom-Back
            wX + 0.5f, wY - 0.5f, wZ + 0.5f,          endU,   startV,  (float) ao1, // Bottom-Front
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   endV,    (float) ao2, // Top-Front
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   endV,    (float) ao2, // Top-Front
            wX + 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,    (float) ao3, // Top-Back
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV,  (float) ao0 // Bottom-Back
        };

        meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
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

        float face[] = {
            wX - 0.5f, wY - 0.5f, wZ - 0.5f, startU, startV, (float) ao0,
            wX - 0.5f, wY - 0.5f, wZ + 0.5f, endU,   startV, (float) ao1,
            wX - 0.5f, wY + 0.5f, wZ + 0.5f, endU,   endV,   (float) ao2,
            wX - 0.5f, wY + 0.5f, wZ + 0.5f, endU,   endV,   (float) ao2,
            wX - 0.5f, wY + 0.5f, wZ - 0.5f, startU, endV,   (float) ao3,
            wX - 0.5f, wY - 0.5f, wZ - 0.5f, startU, startV, (float) ao0
        };

        meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
    }
    // checks front of the block and creates a face and maps the textures
    if (localZ == CHUNK_DEPTH - 1 || manager->isTransparent(getBlock(localX, localY, localZ + 1))) {
        uvs = manager->uvCalculator(blockID, FACE_FRONT);

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_FRONT, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_FRONT, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_FRONT, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_FRONT, 3); // Top-Left Corner

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        float face[] = {
            wX - 0.5f, wY - 0.5f, wZ + 0.5f,          startU, startV, (float) ao0,
            wX + 0.5f, wY - 0.5f, wZ + 0.5f,          endU,   startV, (float) ao1,
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   endV,   (float) ao2,
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   endV,   (float) ao2,
            wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, endV,   (float) ao3,
            wX - 0.5f, wY - 0.5f, wZ + 0.5f,          startU, startV, (float) ao0
        };

        meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
    }
    // checks back of the block and creates a face and maps the textures
    if (localZ == 0 || manager->isTransparent(getBlock(localX, localY, localZ - 1))) {
        uvs = manager->uvCalculator(blockID, FACE_BACK);

        int ao0 = generateAmbientOcclusion(localX, localY, localZ, FACE_BACK, 0); // Bottom-Left Corner
        int ao1 = generateAmbientOcclusion(localX, localY, localZ, FACE_BACK, 1); // Bottom-Right Corner
        int ao2 = generateAmbientOcclusion(localX, localY, localZ, FACE_BACK, 2); // Top-Right Corner
        int ao3 = generateAmbientOcclusion(localX, localY, localZ, FACE_BACK, 3); // Top-Left Corner

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        float face[] = {
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV, (float) ao0,
            wX - 0.5f, wY - 0.5f, wZ - 0.5f,          endU,   startV, (float) ao1,
            wX - 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   (float) ao2,
            wX - 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   (float) ao2,
            wX + 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,   (float) ao3,
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV, (float) ao0
        };

        meshVertices.insert(meshVertices.end(), std::begin(face), std::end(face));
    }
}