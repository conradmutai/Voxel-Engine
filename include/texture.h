#pragma once
#include <string>
#include <glad/glad.h>

class Texture {
public:
    Texture(std::string filepath);

    // Destructor (you will eventually want this to call glDeleteTextures)
    ~Texture();
    void bind();
    unsigned int ID;
};