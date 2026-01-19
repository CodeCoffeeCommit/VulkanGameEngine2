// src/core/Application.h
//
// APPLICATION - Main entry point and coordinator
//
// NEW ARCHITECTURE:
// - Main thread: Window messages, input, UI logic, game logic
// - Render thread: All Vulkan operations (owned by RenderThread)
//
// Main thread NEVER blocks. All GPU work happens on render thread.
//

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
    class Window;
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

    // Prepare frame data for render thread
    libre::FrameData prepareFrameData();

    // ========================================================================
    // RESIZE HANDLING
    // ========================================================================
    void handleResizeComplete();
    bool isMinimized() const;

    // ========================================================================
    // SCENE MANAGEMENT
    // ========================================================================
    void updateTransforms();
    void syncECSToRenderer();
    void handleSelection();

    // ========================================================================
    // UI
    // ========================================================================
    void openPreferencesWindow();

    // ========================================================================
    // CORE COMPONENTS (Owned by Main Thread)
    // ========================================================================
    std::unique_ptr<Window> window;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<Camera> camera;

    // ========================================================================
    // RENDER THREAD (Owns all Vulkan objects)
    // ========================================================================
    std::unique_ptr<libre::RenderThread> renderThread;

    // ========================================================================
    // UI SYSTEM (Logic on main thread, draw commands to render thread)
    // ========================================================================
    std::unique_ptr<libre::ui::UIManager> uiManager;
    libre::ui::Window* preferencesWindow = nullptr;

    // ========================================================================
    // TIMING
    // ========================================================================
    std::chrono::steady_clock::time_point lastFrameTime;
    std::chrono::steady_clock::time_point startTime;
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    float fps = 0.0f;
    uint64_t frameNumber = 0;

    // ========================================================================
    // INPUT STATE
    // ========================================================================
    bool middleMouseDown = false;
    bool shiftHeld = false;
    bool ctrlHeld = false;
    bool altHeld = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    // ========================================================================
    // RESIZE STATE
    // ========================================================================
    std::atomic<bool> pendingResize{ false };
    uint32_t pendingResizeWidth = 0;
    uint32_t pendingResizeHeight = 0;

    // ========================================================================
    // CONSTANTS
    // ========================================================================
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Libre DCC Tool - 3D Viewport";
};