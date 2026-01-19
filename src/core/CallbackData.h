
// src/core/CallbackData.h
#pragma once

#include <functional>  // THIS LINE WAS MISSING!

class Window;
class InputManager;

// Shared data structure for GLFW callbacks
struct CallbackData {
    Window* window = nullptr;
    InputManager* inputManager = nullptr;

    // Called during window drag/resize to keep rendering
    std::function<void()> onRefresh = nullptr;  // THIS LINE WAS MISSING!
};