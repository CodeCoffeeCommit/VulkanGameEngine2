// src/core/CallbackData.h
#pragma once

#include <functional>

class Window;
class InputManager;

// Shared data structure for GLFW callbacks
// Solves the problem of multiple systems needing the user pointer
struct CallbackData {
    Window* window = nullptr;
    InputManager* inputManager = nullptr;

    // Called during window drag/resize to keep rendering
    std::function<void()> onRefresh = nullptr;
};