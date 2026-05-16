#define GLFW_INCLUDE_NONE
#include <glfw/glfw3.h>
#include <glad/glad.h>
#include <libgen.h>

#include "window.h"
#include "shader.h"
#include "stb_image.h"
#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/geometric.hpp>

Window::Window(int width, int height, const char* title) {
    // intialize GLFW
    glfwInit();

    // Set the version to GLFW 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // ESSENTIAL FOR MAC

    // Initalize the window
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
        std::cout << "FAILED TO CREATE WINDOW" <<  std::endl;
        glfwTerminate;
        return;
    }
    glfwMakeContextCurrent(window); // make the current context the window we created for glfw meaning it is targetting this

    // Checks if GLAD can be initializes
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }

    // Prevents permeability
    glEnable(GL_DEPTH);
}


// terminates the window
Window::~Window() {
    glfwTerminate();
}

// function to close window
bool Window::shouldClose() {
    return glfwWindowShouldClose(window);
}

// swaps buffers every loop to join triangles
void Window::swapBuffers() {
    glfwSwapBuffers(window);
}

void Window::pollEvents() {
    glfwPollEvents();
}