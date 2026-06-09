#pragma once
#define RAYCASTER_H

#include "chunk.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

const int MAX_STEPS = 128;
const glm::vec3 GRID_SIZE = glm::vec3(CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_DEPTH);

class ChunkManager;

class RayCaster {
    public:
        RayCaster();
        ~RayCaster();

        // Cast a ray from rayOrigin along rayDir (normalized). Returns world-space t of
        // the first solid voxel hit, or 1e30f if nothing is hit within MAX_STEPS.
        float findNearest(const glm::vec3& rayOrigin, const glm::vec3& rayDir, ChunkManager* world) const;

        float rayAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& rayOrigin, const glm::vec3& rayDestinaton) const;
    private:
        float minimum(const float floatOne, const float floatTwo) const;
        float maximum(const float floatOne, const float floatTwo) const;
};