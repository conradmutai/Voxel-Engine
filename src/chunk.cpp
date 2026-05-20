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
    vertexCount = meshVertices.size() / 5;

    // binds the vertex array output
    glBindVertexArray(VAO);

    // binds the buffer and the meshvertices to the vertex buffer output
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_DYNAMIC_DRAW); // changed from GL_STATIC_DRAW to GL_DYNAMIC_DRAW to provide dynamic buffer updates and performance updates

    // maps the vertex attribute array for both the points of the vertex coords and the texture coords
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    isDirty = false;
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

        float face[] = {
            // X, Y, Z                                // U, V
            wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, startV, // Bottom-Left
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   startV, // Bottom-Right
            wX + 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   // Top-Right
            wX + 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,   // Top-Right
            wX - 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,   // Top-Left
            wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, startV  // Bottom-Left
        };

        meshVertices.insert(meshVertices.end(), face, face + 30);
    }
    // checks below the block and creates a face
    if (localX == 0 || manager->isTransparent(getBlock(localX, localY - 1, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_BOTTOM);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        float face[] = {
            wX - 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV,
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          endU,   startV,
            wX + 0.5f, wY - 0.5f, wZ + 0.5f,          endU,   endV,
            wX + 0.5f, wY - 0.5f, wZ + 0.5f,          endU,   endV,
            wX - 0.5f, wY - 0.5f, wZ + 0.5f,          startU, endV,
            wX - 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV
        };

        meshVertices.insert(meshVertices.end(), face, face + 30);
    }
    // checks right of the block and creates a face and maps the textures
    if (localX == CHUNK_WIDTH + 1 || manager->isTransparent(getBlock(localX + 1, localY, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_RIGHT);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        float face[] = {
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV, // Bottom-Back
            wX + 0.5f, wY - 0.5f, wZ + 0.5f,          endU,   startV, // Bottom-Front
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   endV,   // Top-Front
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   endV,   // Top-Front
            wX + 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,   // Top-Back
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV  // Bottom-Back
        };

        meshVertices.insert(meshVertices.end(), face, face + 30);
    }
    // checks left of the block and creates a face and maps the textures
    if (localX == 0 || manager->isTransparent(getBlock(localX - 1, localY, localZ))) {
        uvs = manager->uvCalculator(blockID, FACE_LEFT);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        float face[] = {
            wX - 0.5f, wY - 0.5f, wZ - 0.5f, startU, startV,
            wX - 0.5f, wY - 0.5f, wZ + 0.5f, endU,   startV,
            wX - 0.5f, wY + 0.5f, wZ + 0.5f, endU,   endV,
            wX - 0.5f, wY + 0.5f, wZ + 0.5f, endU,   endV,
            wX - 0.5f, wY + 0.5f, wZ - 0.5f, startU, endV,
            wX - 0.5f, wY - 0.5f, wZ - 0.5f, startU, startV
        };

        meshVertices.insert(meshVertices.end(), face, face + 30);
    }
    // checks front of the block and creates a face and maps the textures
    if (localZ == CHUNK_DEPTH - 1 || manager->isTransparent(getBlock(localX, localY, localZ + 1))) {
        uvs = manager->uvCalculator(blockID, FACE_FRONT);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        float face[] = {
            wX - 0.5f, wY - 0.5f, wZ + 0.5f,          startU, startV,
            wX + 0.5f, wY - 0.5f, wZ + 0.5f,          endU,   startV,
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   endV,
            wX + 0.5f, wY + 0.5f, wZ + 0.5f,          endU,   endV,
            wX - 0.5f, wY + 0.5f, wZ + 0.5f,          startU, endV,
            wX - 0.5f, wY - 0.5f, wZ + 0.5f,          startU, startV
        };

        meshVertices.insert(meshVertices.end(), face, face + 30);
    }
    // checks back of the block and creates a face and maps the textures
    if (localZ == 0 || manager->isTransparent(getBlock(localX, localY, localZ - 1))) {
        uvs = manager->uvCalculator(blockID, FACE_BACK);

        startU = uvs[0];
        startV = uvs[1];
        endU = uvs[2];
        endV = uvs[3];

        float face[] = {
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV,
            wX - 0.5f, wY - 0.5f, wZ - 0.5f,          endU,   startV,
            wX - 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,
            wX - 0.5f, wY + 0.5f, wZ - 0.5f,          endU,   endV,
            wX + 0.5f, wY + 0.5f, wZ - 0.5f,          startU, endV,
            wX + 0.5f, wY - 0.5f, wZ - 0.5f,          startU, startV
        };

        meshVertices.insert(meshVertices.end(), face, face + 30);
    }
}