// src/core/Application.h
// COMPLETE FILE - Replace your existing Application.h with this

#pragma once

#include "Window.h"
#include "InputManager.h"
#include "Camera.h"
#include "FrameData.h"
#include <memory>
#include <chrono>
#include <atomic>

// Forward declarations
namespace libre {
    class RenderThread;
}

namespace libre::ui {
    class UIManager;
}

class Application {
public:
    Application();
    ~Application();
    void run();

private:
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    void init();
    void cleanup();
    void setupUI();
    void createDefaultScene();
    void printControls();

    // ========================================================================
    // MAIN LOOP (Runs on Main Thread)
    // ========================================================================
    void mainLoop();

    // Process Windows messages and input
    void processInput(float deltaTime);

    // Update game/editor state
    void update(float deltaTime);

    // Update transform hierarchy
    void updateTransforms();

    // Prepare frame data for render thread
    libre::FrameData prepareFrameData();

    // ========================================================================
    // SELECTION
    // ========================================================================
    void handleSelection();

    // ========================================================================
    // RESIZE HANDLING
    // ========================================================================
    bool isMinimized() const;

    // ========================================================================
    // CORE OBJECTS (Main Thread Ownership)
    // ========================================================================
    std::unique_ptr<Window> window;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<libre::ui::UIManager> uiManager;

    // ========================================================================
    // RENDER THREAD (Owns all Vulkan objects)
    // ========================================================================
    std::unique_ptr<libre::RenderThread> renderThread;

    // ========================================================================
    // RESIZE STATE
    // ========================================================================
    std::atomic<bool> pendingResize{ false };

    // ========================================================================
    // TIMING
    // ========================================================================
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastFrameTime;
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    uint64_t frameNumber = 0;

    // ========================================================================
    // INPUT STATE
    // ========================================================================
    bool middleMouseDown = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    bool shiftHeld = false;
    bool ctrlHeld = false;
    bool altHeld = false;
};