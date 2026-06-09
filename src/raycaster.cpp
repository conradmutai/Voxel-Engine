#include "raycaster.h"
#include "chunkmanager.h"

#include <cmath>
#include <algorithm>

inline int getsign(const float f) { return 1 - (int)(((unsigned int&)f) >> 31) * 2; }

inline glm::vec3 sign_of_dir(const glm::vec3& v) {
    return glm::vec3(getsign(v.x), getsign(v.y), getsign(v.z));
}

RayCaster::RayCaster() {

}

RayCaster::~RayCaster() {

}

float RayCaster::rayAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& rayOrigin, const glm::vec3& rayDest) const {
    float tMin = 0, tMax = 1e30f;

    for (int axis = 0; axis < 3; ++axis) {
        const float t1 = (min[axis] - rayOrigin[axis]) / rayDest[axis];
        const float t2 = (max[axis] - rayOrigin[axis]) / rayDest[axis];

        const float dMin = minimum(t1, t2);
        const float dMax = maximum(t1, t2);

        tMin = maximum(dMin, tMin);
        tMax = minimum(dMax, tMax);
    }

    if (tMax >= tMin) return tMin;

    return 1e30f;
}

float RayCaster::findNearest(const glm::vec3& rayOrigin, const glm::vec3& rayDes, ChunkManager* world) const {
    const float CHUNK_SIZE = 16.0f;
    const glm::vec3 grid_min = glm::vec3(
        std::floor(rayOrigin.x / CHUNK_SIZE) * CHUNK_SIZE,
        0.0f,
        std::floor(rayOrigin.z / CHUNK_SIZE) * CHUNK_SIZE
    );
    const glm::vec3 grid_max = grid_min + glm::vec3(CHUNK_SIZE, (float)CHUNK_HEIGHT, CHUNK_SIZE);

    const float entry_t = rayAABB(grid_min, grid_max, rayOrigin, rayDes);

    if (entry_t == 1e30f)
        return 1e30f;

    const glm::vec3 voxelsPerUnit = GRID_SIZE / (grid_max - grid_min);
    const glm::vec3 entryPos = ((rayOrigin + rayDes * (entry_t + 0.0001f)) - grid_min) * voxelsPerUnit;

    const glm::vec3 step = sign_of_dir(rayDes);
    const glm::vec3 delta = glm::abs(glm::vec3(1.0f) / rayDes);

    glm::vec3 pos = glm::clamp(glm::floor(entryPos), glm::vec3(0.0f), GRID_SIZE - glm::vec3(1.0f));
    glm::vec3 tMax = (pos - entryPos + glm::max(step, glm::vec3(0.0f))) / rayDes;

    int axis = 0;
    for (int steps = 0; steps < MAX_STEPS; ++steps) {
        if (world->getBlockWorld((int)(grid_min.x + pos.x), (int)pos.y, (int)(grid_min.z + pos.z)) != 0) {
            if (steps == 0)
                return entry_t;
            return entry_t + (tMax[axis] - delta[axis]) / voxelsPerUnit[axis];
        }

        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                pos.x += step.x;
                if (pos.x < 0 || pos.x >= CHUNK_WIDTH)
                    break;
                axis = 0;
                tMax.x += delta.x;
            } else {
                pos.z += step.z;
                if (pos.z < 0 || pos.z >= CHUNK_DEPTH)
                    break;
                axis = 2;
                tMax.z += delta.z;
            }
        } else {
            if (tMax.y < tMax.z) {
                pos.y += step.y;
                if (pos.y < 0 || pos.y >= CHUNK_HEIGHT)
                    break;
                axis = 1;
                tMax.y += delta.y;
            } else {
                pos.z += step.z;
                if (pos.z < 0 || pos.z >= CHUNK_DEPTH)
                    break;
                axis = 2;
                tMax.z += delta.z;
            }
        }
    }
    return 1e30f;
}

float RayCaster::minimum(const float floatOne, const float floatTwo) const {
    if (floatOne > floatTwo) {
        return floatTwo;
    } else {
        return floatOne;
    }
}

float RayCaster::maximum(const float floatOne, const float floatTwo) const {
    if (floatOne > floatTwo) {
        return floatOne;
    } else {
        return floatTwo;
    }
}
