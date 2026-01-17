#include "InputManager.h"
#include "Window.h"
#include "CallbackData.h"
#include <iostream>

InputManager::InputManager(Window* window) : window(window) {
    glfwWindow = window->getHandle();

    // Register this InputManager in the shared callback data
    // (Window already set the user pointer, we just add ourselves to it)
    CallbackData* callbackData = window->getCallbackData();
    callbackData->inputManager = this;

    // Set callbacks - they will use the shared CallbackData
    glfwSetKeyCallback(glfwWindow, keyCallback);
    glfwSetMouseButtonCallback(glfwWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(glfwWindow, cursorPositionCallback);
    glfwSetScrollCallback(glfwWindow, scrollCallback);

    // Get initial mouse position
    glfwGetCursorPos(glfwWindow, &mouseX, &mouseY);
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    std::cout << "[OK] Input manager initialized" << std::endl;
}

InputManager::~InputManager() {
    // Unregister from callback data
    if (window) {
        CallbackData* callbackData = window->getCallbackData();
        if (callbackData) {
            callbackData->inputManager = nullptr;
        }
    }
    std::cout << "[OK] Input manager destroyed" << std::endl;
}

void InputManager::update() {
    // Clear just pressed/released states
    keysJustPressed.clear();
    keysJustReleased.clear();
    mouseButtonsJustPressed.clear();
    mouseButtonsJustReleased.clear();

    // Update mouse delta
    mouseDeltaX = mouseX - lastMouseX;
    mouseDeltaY = mouseY - lastMouseY;
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    // Reset scroll (scroll is event-based, not state-based)
    scrollX = 0.0;
    scrollY = 0.0;
}

bool InputManager::isKeyPressed(int key) const {
    auto it = keysPressed.find(key);
    return it != keysPressed.end() && it->second;
}

bool InputManager::isKeyJustPressed(int key) const {
    auto it = keysJustPressed.find(key);
    return it != keysJustPressed.end() && it->second;
}

bool InputManager::isKeyJustReleased(int key) const {
    auto it = keysJustReleased.find(key);
    return it != keysJustReleased.end() && it->second;
}

bool InputManager::isMouseButtonPressed(int button) const {
    auto it = mouseButtonsPressed.find(button);
    return it != mouseButtonsPressed.end() && it->second;
}

bool InputManager::isMouseButtonJustPressed(int button) const {
    auto it = mouseButtonsJustPressed.find(button);
    return it != mouseButtonsJustPressed.end() && it->second;
}

bool InputManager::isMouseButtonJustReleased(int button) const {
    auto it = mouseButtonsJustReleased.find(button);
    return it != mouseButtonsJustReleased.end() && it->second;
}

// Static callbacks - now use shared CallbackData
void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
    if (!data || !data->inputManager) return;

    InputManager* input = data->inputManager;

    if (action == GLFW_PRESS) {
        input->keysPressed[key] = true;
        input->keysJustPressed[key] = true;
    }
    else if (action == GLFW_RELEASE) {
        input->keysPressed[key] = false;
        input->keysJustReleased[key] = true;
    }
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
    if (!data || !data->inputManager) return;

    InputManager* input = data->inputManager;

    if (action == GLFW_PRESS) {
        input->mouseButtonsPressed[button] = true;
        input->mouseButtonsJustPressed[button] = true;
    }
    else if (action == GLFW_RELEASE) {
        input->mouseButtonsPressed[button] = false;
        input->mouseButtonsJustReleased[button] = true;
    }
}

void InputManager::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
    if (!data || !data->inputManager) return;

    InputManager* input = data->inputManager;

    input->mouseX = xpos;
    input->mouseY = ypos;

    if (input->firstMouse) {
        input->lastMouseX = xpos;
        input->lastMouseY = ypos;
        input->firstMouse = false;
    }
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
    if (!data || !data->inputManager) return;

    InputManager* input = data->inputManager;

    input->scrollX = xoffset;
    input->scrollY = yoffset;
}