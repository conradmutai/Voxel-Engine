#include "chunk.h"

Chunk::Chunk(int gridX, int gridZ) : gridX(gridX), gridZ(gridZ), isDirty(true), vertexCount(0) {
    // 1. Heap Allocation for the 1D Array (Initialized to 0/AIR)
    blocks = new uint8_t[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH]();

    // 2. Generate Basic Flat Terrain 
    for (int z = 0; z < CHUNK_DEPTH; ++z) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < CHUNK_WIDTH; ++x) {
                if(y == 15) {
                    setBlock(x, y, z, GRASS);
                } else {
                    setBlock(x, y, z, DIRT);
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
    std::vector<float> meshVertices;
    
    for (int z = 0; z < CHUNK_DEPTH; z++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                if (getBlock(x, y, z) != AIR) {
                    addBlockVertices(meshVertices, x, y, z);
                }
            }
        }
    }

    vertexCount = meshVertices.size() / 5;

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    isDirty = false;
}

void Chunk::addBlockVertices(std::vector<float>& vertices, int localX, int localY, int localZ) {
    // Convert local chunk coordinates to world coordinates
    float wX = (gridX * CHUNK_WIDTH) + localX;
    float wY = localY;
    float wZ = (gridZ * CHUNK_DEPTH) + localZ;

    float blockVertices[] = {
        wX-0.5f, wY-0.5f, wZ-0.5f,  0.0f, 0.0f,
        wX+0.5f, wY-0.5f, wZ-0.5f,  1.0f, 0.0f,
        wX+0.5f, wY+0.5f, wZ-0.5f,  1.0f, 1.0f,
        wX+0.5f, wY+0.5f, wZ-0.5f,  1.0f, 1.0f,
        wX-0.5f, wY+0.5f, wZ-0.5f,  0.0f, 1.0f,
        wX-0.5f, wY-0.5f, wZ-0.5f,  0.0f, 0.0f,

        wX-0.5f, wY-0.5f, wZ+0.5f,  0.0f, 0.0f,
        wX+0.5f, wY-0.5f, wZ+0.5f,  1.0f, 0.0f,
        wX+0.5f, wY+0.5f, wZ+0.5f,  1.0f, 1.0f,
        wX+0.5f, wY+0.5f, wZ+0.5f,  1.0f, 1.0f,
        wX-0.5f, wY+0.5f, wZ+0.5f,  0.0f, 1.0f,
        wX-0.5f, wY-0.5f, wZ+0.5f,  0.0f, 0.0f,

        wX-0.5f, wY+0.5f, wZ+0.5f,  1.0f, 0.0f,
        wX-0.5f, wY+0.5f, wZ-0.5f,  1.0f, 1.0f,
        wX-0.5f, wY-0.5f, wZ-0.5f,  0.0f, 1.0f,
        wX-0.5f, wY-0.5f, wZ-0.5f,  0.0f, 1.0f,
        wX-0.5f, wY-0.5f, wZ+0.5f,  0.0f, 0.0f,
        wX-0.5f, wY+0.5f, wZ+0.5f,  1.0f, 0.0f,

        wX+0.5f, wY+0.5f, wZ+0.5f,  1.0f, 0.0f,
        wX+0.5f, wY+0.5f, wZ-0.5f,  1.0f, 1.0f,
        wX+0.5f, wY-0.5f, wZ-0.5f,  0.0f, 1.0f,
        wX+0.5f, wY-0.5f, wZ-0.5f,  0.0f, 1.0f,
        wX+0.5f, wY-0.5f, wZ+0.5f,  0.0f, 0.0f,
        wX+0.5f, wY+0.5f, wZ+0.5f,  1.0f, 0.0f,

        wX-0.5f, wY-0.5f, wZ-0.5f,  0.0f, 1.0f,
        wX+0.5f, wY-0.5f, wZ-0.5f,  1.0f, 1.0f,
        wX+0.5f, wY-0.5f, wZ+0.5f,  1.0f, 0.0f,
        wX+0.5f, wY-0.5f, wZ+0.5f,  1.0f, 0.0f,
        wX-0.5f, wY-0.5f, wZ+0.5f,  0.0f, 0.0f,
        wX-0.5f, wY-0.5f, wZ-0.5f,  0.0f, 1.0f,

        wX-0.5f, wY+0.5f, wZ-0.5f,  0.0f, 1.0f,
        wX+0.5f, wY+0.5f, wZ-0.5f,  1.0f, 1.0f,
        wX+0.5f, wY+0.5f, wZ+0.5f,  1.0f, 0.0f,
        wX+0.5f, wY+0.5f, wZ+0.5f,  1.0f, 0.0f,
        wX-0.5f, wY+0.5f, wZ+0.5f,  0.0f, 0.0f,
        wX-0.5f, wY+0.5f, wZ-0.5f,  0.0f, 1.0f
    };

    vertices.insert(vertices.end(), std::begin(blockVertices), std::end(blockVertices));
}