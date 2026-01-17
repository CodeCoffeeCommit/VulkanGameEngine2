#include "Window.h"
#include <stdexcept>
#include <iostream>

Window::Window(int width, int height, const std::string& title)
    : width(width), height(height), title(title) {

    // Initialize GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    // Configure GLFW for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create window
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
    }

    // Set up callback data - Window registers itself
    callbackData.window = this;

    // Set user pointer to shared callback data
    glfwSetWindowUserPointer(window, &callbackData);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    std::cout << "[OK] Window created (" << width << "x" << height << ")" << std::endl;
}

Window::~Window() {
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

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    // Get the shared callback data
    auto* data = reinterpret_cast<CallbackData*>(glfwGetWindowUserPointer(window));
    if (data && data->window) {
        data->window->framebufferResized = true;
        data->window->width = width;
        data->window->height = height;
    }
}