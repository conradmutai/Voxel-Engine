#define GLFW_INCLUDE_NONE
#include <glfw/glfw3.h>
#include <glad/glad.h>
#include <libgen.h>
#include <mach-o/dyld.h>

#include "shader.h"
#include "stb_image.h"
#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/geometric.hpp>

// helper method to get file path
std::string getExecutableDir() {
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::string(dirname(path)) + "/";
    }
    return "./";
};

int main() {
    std::string base = getExecutableDir();
    Shader ourShader((base + "shader.vs").c_str(), (base + "shader.fs").c_str());

    float vertices[] {

    };
    unsigned int indices[] {
        0, 1, 3,
        1, 2, 3
    };
    
    unsigned int VBO, VAO, EBO;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, VBO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, EBO);

    
}