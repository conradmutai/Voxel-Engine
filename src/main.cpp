#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <libgen.h>
#include <mach-o/dyld.h>

#include "blockmanager.h"
#include "shader.h"
#include "stb_image.h"
#include "camera.h"
#include "texture.h"
#include "window.h"
#include "chunk.h"

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

// Global Camera and Time state
Camera camera(glm::vec3(0.0f, 16.0f, 3.0f));
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// Global Mouse state
bool firstMouse = true;
float lastX = 800.0f / 2.0f; // Center of your Retina screen
float lastY = 600.0f / 2.0f;

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

int main() {
    // 1. Creates window
    Window window(800, 600, "VOXEL ENGINE - VERSION 1.0.0");

    glfwSetInputMode(window.getRawPointer(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window.getRawPointer(), mouse_callback);
    glfwSetScrollCallback(window.getRawPointer(), scroll_callback);

    // 2. Establish base file path
    std::string base = getExecutableDir();

    // 3. Loads shaders and the file path
    Shader ourShader((base + "shader.vs").c_str(), (base + "shader.fs").c_str());

    // 4. Loads textures
    Texture texture(base + "resources/terrain.png");

    // 5. Initiates the blockmanager object
    BlockManager manager;

    // 6. Initiates new chunk
    Chunk chunk(0,0,&manager);

    // 7. game loop
    while(!window.shouldClose()) {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process keyboard inputs
        processInput(window.getRawPointer());

        // Render clearing
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind Shader
        ourShader.use();
        ourShader.setInt("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        texture.bind(); // Your loaded atlas.png
        chunk.render();

        // Pass matrices to shader
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f); // Static cube at 0,0,0
        ourShader.setMat4("model", model);

        chunk.render();

        // Swap buffers & poll events
        window.swapBuffers();
        window.pollEvents();
    }

    return 0;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since Y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow *window) {
    // Exit game with Escape
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD Movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}