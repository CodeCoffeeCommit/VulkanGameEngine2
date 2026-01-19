// src/core/Window.h
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <functional>
#include "CallbackData.h"

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    // Prevent copying
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Window operations
    bool shouldClose() const;
    void pollEvents();
    VkExtent2D getExtent() const;
    GLFWwindow* getHandle() const { return window; }

    // Resize handling
    bool wasResized() const { return framebufferResized; }
    void resetResizeFlag() { framebufferResized = false; }

    // Set callback for rendering during drag/resize
    void setRefreshCallback(std::function<void()> callback);

    // Callback data access (for InputManager to register)
    CallbackData* getCallbackData() { return &callbackData; }

    // Get current dimensions
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void windowRefreshCallback(GLFWwindow* window);

    GLFWwindow* window = nullptr;
    int width;
    int height;
    std::string title;
    bool framebufferResized = false;

    // Shared callback data - Window owns this
    CallbackData callbackData;
};