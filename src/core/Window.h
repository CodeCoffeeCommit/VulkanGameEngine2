// src/core/Window.h
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <functional>
#include <atomic>
#include "CallbackData.h"

// Windows-specific includes for native window handle
#ifdef _WIN32
#define NOMINMAX                 // Prevent Windows.h min/max macros
#define WIN32_LEAN_AND_MEAN      // Exclude rarely-used Windows stuff
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>
#endif

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
    // Note: With render thread, this is mainly for signaling, not actual rendering
    void setRefreshCallback(std::function<void()> callback);

    // Callback data access (for InputManager to register)
    CallbackData* getCallbackData() { return &callbackData; }

    // Get current dimensions
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Check if currently in modal resize/move loop (Windows-specific)
    bool isInModalLoop() const { return inModalLoop_; }

    // Check if currently resizing (NEW)
    bool isResizing() const { return isResizing_; }

    // Allow Application to set framebufferResized directly
    bool framebufferResized = false;

private:
    // GLFW callbacks
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void windowRefreshCallback(GLFWwindow* window);
    static void windowPosCallback(GLFWwindow* window, int xpos, int ypos);

#ifdef _WIN32
    // Windows-specific: subclass procedure to catch modal loop
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    WNDPROC originalWndProc_ = nullptr;
    void setupWindowsHook();
#endif

    GLFWwindow* window = nullptr;
    int width;
    int height;
    std::string title;

    // Track modal loop state
    std::atomic<bool> inModalLoop_{ false };

    // Track resize state (NEW - for render thread architecture)
    std::atomic<bool> isResizing_{ false };

    // Shared callback data - Window owns this
    CallbackData callbackData;
};