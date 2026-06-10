#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>

// default camera movement
const float PITCH = 0.0f;
const float YAW = -90.0f;
const float SPEED = 15.0f;
const float SENSITIVITY = 0.2f;
const float ZOOM = 45.0f;

// defines possible camera movements
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// A hit object will be created for each collision that occurs, also made as a struct that means it doenst operate as a function
struct Hit {
    public:
        glm::ivec3 collider; // the world coordinate
        glm::vec3 pos; // the position coord
        glm::vec3 delta; // the difference in position
        glm::vec3 normal; // the normalized value
        float time; // the time measurement

    // constructor for the hit object
    Hit(glm::ivec3 collider) {
        this->collider = collider;
        this->pos = glm::vec3();
        this->delta = glm::vec3();
        this->normal = glm::vec3();
        this->time = 0.0f;
    }
};

// AABB struct made to allow the classification of collision with blocks
struct AABB {
    public:
        glm::vec3 pos; // the position in a world sense
        glm::vec3 half; // a half way point on the vector (center)

    // constructor
    AABB(glm::vec3 pos, glm::vec3 half) {
        this->pos = pos;
        this->half = half;
    }

    // function to realize if the intersection occurs leading to the object deflecting
    std::optional<Hit> intersectAABB(AABB box) {
        const float dx = box.pos.x - this->pos.x; // difference between two centers
        const float px = (box.half.x + this->half.x) - glm::abs(dx); // overlap/penetration distance on given axis

        // means that the two objects have no overlap
        if (px <= 0) {
            return std::nullopt;
        }

        const float dy = box.pos.y - this->pos.y;
        const float py = (box.half.y + this->half.y) - glm::abs(dy);

        if (py <= 0) {
            return std::nullopt;
        }

        const float dz = box.pos.z - this->pos.z;
        const float pz = (box.half.z + this->half.z) - glm::abs(dz);

        if (pz <= 0) {
            return std::nullopt;
        }

        Hit hit = Hit(glm::ivec3(this->pos));

        // looks for the smallest overlap and then declares several variables and assigns values
        if (px < py) {
            if (px < pz) {
                const int sx = (dx >= 0) ? 1 : -1; // gets the sign 
                hit.delta.x = -px * sx;
                hit.normal.x = sx;
                hit.pos.x = this->pos.x + (this->half.x * sx);
                hit.pos.y = box.pos.y;
                hit.pos.z = box.pos.z;
            } else {
                const int sz = (dz >= 0) ? 1 : -1;
                hit.delta.z = -pz * sz;
                hit.normal.z = sz;
                hit.pos.x = box.pos.x;
                hit.pos.y = box.pos.y;
                hit.pos.z = this->pos.z + (this->half.z * sz);
            }
        } else {
            if (py < pz) { 
                const int sy = (dy >= 0) ? 1 : -1;
                hit.delta.y = -py * sy;
                hit.normal.y = sy;
                hit.pos.x = box.pos.x;
                hit.pos.y = this->pos.y + (this->half.y * sy);
                hit.pos.z = box.pos.z;
            } else {
                const int sz = (dz >= 0) ? 1 : -1;
                hit.delta.z = -pz * sz;
                hit.normal.z = sz;
                hit.pos.x = box.pos.x;
                hit.pos.y = box.pos.y;
                hit.pos.z = this->pos.z + (this->half.z * sz);
            }
        }

        return hit;
    }
};

class Camera {
    public:
        // camera Attributes
        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;
        // euler Angles
        float Yaw;
        float Pitch;
        // camera options
        float MovementSpeed;
        float MouseSensitivity;
        float Zoom;

        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
            Position = position;
            WorldUp = up;
            Yaw = yaw;
            Pitch = pitch;
            updateCameraVectors();
        }

        Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
            Position = glm::vec3(posX, posY, posZ);
            WorldUp = glm::vec3(upX, upY, upZ);
            Yaw = yaw;
            Pitch = pitch;
            updateCameraVectors();
        }

        glm::mat4 GetViewMatrix()
        {
            return glm::lookAt(Position, Position + Front, Up);
        }
        
        void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
            float cameraSpeed = 2.5f * deltaTime;
            float velocity = MovementSpeed * deltaTime;
            if (direction == FORWARD)
                Position += Front * velocity;
            if (direction == BACKWARD)
                Position -= Front * velocity;
            if (direction == LEFT)
                Position -= Right * velocity;
            if (direction == RIGHT)
                Position += Right * velocity;
        }

        // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
        {
            xoffset *= MouseSensitivity;
            yoffset *= MouseSensitivity;

            Yaw   += xoffset;
            Pitch += yoffset;

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            if (constrainPitch)
            {
                if (Pitch > 89.0f)
                    Pitch = 89.0f;
                if (Pitch < -89.0f)
                    Pitch = -89.0f;
            }

            // update Front, Right and Up Vectors using the updated Euler angles
            updateCameraVectors();
        }

        // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
        void ProcessMouseScroll(float yoffset)
        {
            Zoom -= (float)yoffset;
            if (Zoom < 1.0f)
                Zoom = 1.0f;
            if (Zoom > 45.0f)
                Zoom = 45.0f;
        }

    private:
        // calculates euler angles
        void updateCameraVectors() {
                glm::vec3 front;
                front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
                front.y = sin(glm::radians(Pitch));
                front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
                Front = glm::normalize(front);
                Right = glm::normalize(glm::cross(Front, WorldUp));  
                Up    = glm::normalize(glm::cross(Right, Front));
        }
};

#endif