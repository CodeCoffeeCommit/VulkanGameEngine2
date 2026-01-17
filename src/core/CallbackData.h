// src/core/CallbackData.h
#pragma once

class Window;
class InputManager;

// Shared data structure for GLFW callbacks
// Solves the problem of multiple systems needing the user pointer
struct CallbackData {
    Window* window = nullptr;
    InputManager* inputManager = nullptr;
};