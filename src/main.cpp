#define GLFW_INCLUDE_NONE
#include <glfw/glfw3.h>
#include <glad/glad.h>
#include <libgen.h>
#include <mach-o/dyld.h>

#include "shader.h"
#include "stb_image.h"
#include "camera.h"
#include "texture.h"
#include "window.h"

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

void processInput(Window window);

int main() {
    // 1. Creates window
    Window window = Window(800, 600, "VOXEL ENGINE - VERSION 1.0.0");

    // 2. Establish base file path
    std::string base = getExecutableDir();

    // 3. Loads shaders and the file path
    Shader ourShader((base + "shader.vs").c_str(), (base + "shader.fs").c_str());

    // 4. Loads textures
    Texture dirtTexture(base + "dirt.png");

    // 5. Create cubes and indices
    float vertices[] {
        // vertex points       // texture coords
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,      // face 1, bottom left corner, triangle 1
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,      // face 1, top left corner, triangle 1
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,      // face 1, bottom right corner, triangle 1
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,      // face 1, bottom right corner, triagnle 2
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,      // face 1, top right corner, triangle 2
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,      // face 1, top left corner, triangle 2

        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,      // face 2, top right corner, triangle 3
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,      // face 2, bottom right corner, triangle 3
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f,      // face 2, bottom left corner, triangle 3
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f,      // face 2, bottom left corner, triangle 4
         0.5f,  0.5f,  0.5f,    1.0f, 1.0f,      // face 2, top left corner, triangle 4
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,      // face 2, top right corner, triangle 4

         0.5f, -0.5f,  0.5f,    1.0f, 0.0f,      // face 3, bottom left corner, triangle 5
         0.5f,  0.5f,  0.5f,    1.0f, 1.0f,      // face 3, top left corner, triangle 5
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,      // face 3, bottom right corner, triangle 5
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,      // face 3, bottom right corner, triangle 6
        -0.5f,  0.5f,  0.5f,    0.0f, 1.0f,      // face 3, top right corner, triangle 6
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f,      // face 3, bottom left corner, triangle 6

         0.5f, -0.5f,  0.5f,    1.0f, 0.0f,      // face 4, bottom left corner, triangle 7
         0.5f,  0.5f,  0.5f,    1.0f, 1.0f,      // face 4, top left corner, triangle 7
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,      // face 4, bottom right corner, triangle 7
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,      // face 4, bottom right corner, triangle 8
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,      // face 4, top right corner, triangle 8
         0.5f,  0.5f,  0.5f,    1.0f, 1.0f,      // face 4, top left corner, triangle 8

         0.5f,  0.5f,  0.5f,    1.0f, 1.0f,      // face 5, triangle 9
        -0.5f,  0.5f,  0.5f,    0.0f, 1.0f,      // face 5, triangle 9
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,      // face 5, triangle 9
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,      // face 5, triangle 10
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,      // face 5, triangle 10
        -0.5f,  0.5f,  0.5f,    0.0f, 1.0f,      // face 5, triangle 10
        
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f,      // face 5, triangle 11
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,      // face 5, triangle 11
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,      // face 5, triangle 11
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,      // face 5, triangle 12
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,      // face 5, triangle 12
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,      // face 5, triangle 12
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

    // 6. game loop
    while(!window.shouldClose()) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind Shader & Matrices
        ourShader.use();

        // (You would calculate View & Projection matrices here)
        

        // Bind Texture & Draw
        dirtTexture.bind();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Swap buffers and poll OS events
        window.swapBuffers();
        window.pollEvents();
    }
}