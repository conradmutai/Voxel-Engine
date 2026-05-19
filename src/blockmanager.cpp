#include "blockmanager.h"

BlockManager::BlockManager() {

}

// looks up the atlas coords and calculates the floating-point UV corners to return back tot hechunk
std::vector<float> BlockManager::uvCalculator(int BlockID, enum faceDirection faceDir) {
    std::vector<float> UV;

    int column = 0, row = 0;

     switch(BlockID) {
        case DIRT: 
            column = 2; 
            row = 0;
            break;
        case STONE: 
            column = 1; 
            row = 0;
            break;
        case GRASS:
            // grass has a different top texture so it takes this factor into account
            switch(faceDir) {
                case FACE_TOP:
                    column = 0;
                    row = 0;
                    break;
                case FACE_BOTTOM:
                    column = 2;
                    row = 0;
                    break;
                default:
                    column = 3;
                    row = 0;
                    break;
            }
            break;
        default: 
            column = 0;
            row = 0;
            break;
    }

    float startU, startV, endU, endV;

    startU = column / 16.0;
    startV = row / 16.0;

    endU = startU + (1.0/16.0);
    endV = startV + (1.0/16.0);

    UV.push_back(startU);
    UV.push_back(startV);
    UV.push_back(endU);
    UV.push_back(endV);
    
    return UV;
}