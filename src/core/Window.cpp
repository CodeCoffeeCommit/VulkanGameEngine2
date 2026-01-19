// src/core/Window.cpp
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
        self->inModalLoop_ = true;
        break;

    case WM_EXITSIZEMOVE:
        self->inModalLoop_ = false;
        break;

    case WM_MOVING:
    case WM_SIZING:
        // Render during drag/resize
        if (self->callbackData.onRefresh) {
            self->callbackData.onRefresh();
        }
        break;

    case WM_PAINT:
        if (self->inModalLoop_ && self->callbackData.onRefresh) {
            self->callbackData.onRefresh();
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
        std::cout << "[OK] Windows modal loop hook installed" << std::endl;
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
        data->window->framebufferResized = true;
        data->window->width = w;
        data->window->height = h;

        // Immediate refresh during resize
        if (data->onRefresh) {
            data->onRefresh();
        }
    }
}

void Window::windowRefreshCallback(GLFWwindow* glfwWin) {
    auto* data = reinterpret_cast<CallbackData*>(glfwGetWindowUserPointer(glfwWin));
    if (data && data->onRefresh) {
        data->onRefresh();
    }
}

void Window::windowPosCallback(GLFWwindow* glfwWin, int xpos, int ypos) {
    (void)xpos; (void)ypos;
    auto* data = reinterpret_cast<CallbackData*>(glfwGetWindowUserPointer(glfwWin));
    if (data && data->window && data->onRefresh) {
        if (data->window->isInModalLoop()) {
            data->onRefresh();
        }
    }
}