#pragma once 
#define BLOCKMANAGER_H

#include <vector>
#include <cstdint>

enum BlockType : uint8_t {
    AIR = 0,
    DIRT = 1,
    GRASS = 2,
    STONE = 3,
    WATER = 4, 
    GLASS = 5
};

enum faceDirection : int {
    FACE_TOP = 0,
    FACE_BOTTOM = 1, 
    FACE_LEFT = 2, 
    FACE_RIGHT = 3,
    FACE_FRONT = 4,
    FACE_BACK = 5
};

class BlockManager {
    public:
        BlockManager();
        std::vector<float> uvCalculator(int BlockID, enum faceDirection faceDir);

        // a class that handles logic for transparent block
        bool isTransparent(uint8_t blockID);
};