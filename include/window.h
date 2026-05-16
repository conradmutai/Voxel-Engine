#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

class Window {
public:
    // Constructor 
    Window(int width, int height, const char* title);

    // Destructor
    ~Window();

    // Standard game loop methods
    bool shouldClose();
    void swapBuffers();
    void pollEvents();

    GLFWwindow* getRawPointer() { return window; }

private:
    GLFWwindow* window;
};