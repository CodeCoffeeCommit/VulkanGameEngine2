#pragma once
#include "Window.h"
#include "InputManager.h"
#include "Camera.h"
#include "../render/VulkanContext.h"
#include <memory>
#include <chrono>

// Forward declarations
class SwapChain;
class Renderer;

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
    void init();
    void mainLoop();
    void cleanup();
    void update(float deltaTime);
    void render();

    void handleResize();
    bool isMinimized() const;
    void processInput(float deltaTime);

    void createDefaultScene();
    void updateTransforms();
    void syncECSToRenderer();
    void handleSelection();
    void printControls();

    // UI Setup
    void setupUI();
    void openPreferencesWindow();

    // Core components
    std::unique_ptr<Window> window;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<Camera> camera;

    // Rendering components
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<Renderer> renderer;

    // UI System
    std::unique_ptr<libre::ui::UIManager> uiManager;
    libre::ui::Window* preferencesWindow = nullptr;

    // Timing
    std::chrono::steady_clock::time_point lastFrameTime;
    float deltaTime = 0.0f;
    float fps = 0.0f;

    // Mouse state
    bool middleMouseDown = false;
    bool shiftHeld = false;
    bool ctrlHeld = false;
    bool altHeld = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    bool framebufferResized = false;

    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Libre DCC Tool - 3D Viewport";
};