// src/core/Window.cpp
//
// RENDER THREAD ARCHITECTURE:
// - WM_MOVING/WM_SIZING no longer trigger rendering
// - Render thread runs independently
// - Swapchain recreation only when resize ENDS
//

#include "Window.h"
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
// Static pointer for Win32 callback (single window app)
static Window* g_windowInstance = nullptr;

LRESULT CALLBACK Window::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* self = g_windowInstance;
    if (!self || !self->originalWndProc_) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_ENTERSIZEMOVE:
        // Entering modal drag/resize loop
        self->inModalLoop_ = true;
        break;

    case WM_EXITSIZEMOVE:
        // Exiting modal drag/resize loop
        self->inModalLoop_ = false;
        self->isResizing_ = false;
        // Signal that resize completed - main loop will request swapchain recreate
        self->framebufferResized = true;
        break;

    case WM_MOVING:
        // Window is being dragged by title bar
        // RENDER THREAD ARCHITECTURE: Do nothing here!
        // The render thread continues rendering independently.
        // This is why window drag is now smooth.
        break;

    case WM_SIZING:
        // Window edge is being dragged to resize
        // RENDER THREAD ARCHITECTURE: Do nothing here!
        // The render thread continues at the old size.
        // When user releases mouse (WM_EXITSIZEMOVE), we recreate swapchain.
        self->isResizing_ = true;
        break;

    case WM_PAINT:
        // Paint messages during modal loop
        // RENDER THREAD ARCHITECTURE: No special handling needed.
        // Render thread is already rendering continuously.
        // Just validate the paint region to prevent more WM_PAINT messages.
        if (self->inModalLoop_) {
            ValidateRect(hwnd, nullptr);
            return 0;
        }
        break;

    case WM_SIZE:
        // Window size changed (also sent during minimize/maximize)
    {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);

        if (wParam == SIZE_MINIMIZED) {
            // Window minimized - render thread will detect this and sleep
        }
        else if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) {
            // Window restored or maximized
            self->width = static_cast<int>(width);
            self->height = static_cast<int>(height);

            // If not in modal loop, this was a programmatic resize
            // (e.g., maximize button) - signal for swapchain recreation
            if (!self->inModalLoop_) {
                self->framebufferResized = true;
            }
        }
    }
    break;
    }

    return CallWindowProc(self->originalWndProc_, hwnd, msg, wParam, lParam);
}

void Window::setupWindowsHook() {
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd) {
        std::cerr << "[Window] Warning: Could not get Win32 handle" << std::endl;
        return;
    }

    g_windowInstance = this;
    originalWndProc_ = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(windowProc))
        );

    if (originalWndProc_) {
        std::cout << "[OK] Windows modal loop hook installed (render thread mode)" << std::endl;
    }
}
#endif

Window::Window(int width, int height, const std::string& title)
    : width(width), height(height), title(title) {

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
    }

    // Setup callbacks
    callbackData.window = this;
    glfwSetWindowUserPointer(window, &callbackData);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetWindowPosCallback(window, windowPosCallback);

#ifdef _WIN32
    setupWindowsHook();
#endif

    std::cout << "[OK] Window created (" << width << "x" << height << ")" << std::endl;
}

Window::~Window() {
#ifdef _WIN32
    if (originalWndProc_) {
        HWND hwnd = glfwGetWin32Window(window);
        if (hwnd) {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWndProc_));
        }
    }
    g_windowInstance = nullptr;
#endif

    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "[OK] Window destroyed" << std::endl;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

VkExtent2D Window::getExtent() const {
    VkExtent2D extent;
    extent.width = static_cast<uint32_t>(width);
    extent.height = static_cast<uint32_t>(height);
    return extent;
}

void Window::setRefreshCallback(std::function<void()> callback) {
    callbackData.onRefresh = callback;
}

void Window::framebufferResizeCallback(GLFWwindow* glfwWin, int w, int h) {
    auto* data = reinterpret_cast<CallbackData*>(glfwGetWindowUserPointer(glfwWin));
    if (data && data->window) {
        data->window->width = w;
        data->window->height = h;

        // RENDER THREAD ARCHITECTURE:
        // Don't call onRefresh here - just mark for resize
        // Main loop will handle swapchain recreation
        data->window->framebufferResized = true;
    }
}

void Window::windowRefreshCallback(GLFWwindow* glfwWin) {
    auto* data = reinterpret_cast<CallbackData*>(glfwGetWindowUserPointer(glfwWin));
    if (data && data->onRefresh) {
        // RENDER THREAD ARCHITECTURE:
        // This callback is now optional - render thread runs independently
        // We keep it for signaling purposes but it shouldn't do heavy work
        data->onRefresh();
    }
}

void Window::windowPosCallback(GLFWwindow* glfwWin, int xpos, int ypos) {
    (void)xpos; (void)ypos;
    // RENDER THREAD ARCHITECTURE:
    // Position changes don't require any special handling
    // Render thread continues independently
}