#pragma once
#include <string>
#include <glad/glad.h>

class Texture {
public:
    Texture(std::string filepath);

    // Destructor 
    ~Texture();
    void bind();
    unsigned int ID;
};