// src/core/Application.cpp
//
// NEW ARCHITECTURE: Main thread never blocks on GPU.
// All rendering happens on dedicated RenderThread.
//

#include "Application.h"
#include "Editor.h"
#include "FrameData.h"
#include "../render/RenderThread.h"
#include "../render/Mesh.h"
#include "../world/Primitives.h"
#include "../ui/UI.h"
#include "../ui/Widgets.h"
#include "../ui/PreferencesWindow.h"
#include "../ShaderCompiler.h"
#include "../ui/UIScale.h"

#include <iostream>
#include <algorithm>

Application::Application() {
    std::cout << "====================================" << std::endl;
    std::cout << "LIBRE DCC TOOL - 3D Viewport" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "[Architecture] Render Thread Enabled" << std::endl;
}

Application::~Application() {
    cleanup();
}

void Application::run() {
    init();
    mainLoop();
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void Application::init() {
    std::cout << "\n=== Initializing Application ===" << std::endl;

    // Record start time
    startTime = std::chrono::steady_clock::now();
    lastFrameTime = startTime;

    // Create window (Main Thread owns this)
    window = std::make_unique<Window>(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    // Create input manager - takes Window* in constructor
    inputManager = std::make_unique<InputManager>(window.get());

    // Initialize Editor singleton
    libre::Editor::instance().initialize();

    // Create camera (Main Thread owns this)
    camera = std::make_unique<Camera>();
    camera->setAspectRatio(static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT));

    // Create and start render thread
    // ALL Vulkan initialization happens on the render thread
    renderThread = std::make_unique<libre::RenderThread>();
    if (!renderThread->start(window.get())) {
        throw std::runtime_error("Failed to start render thread!");
    }

    // Setup window resize callback - just sets a flag, doesn't render
    window->setRefreshCallback([this]() {
        // During modal drag/resize, just mark that we need to handle it
        // DO NOT render here - that would block the message loop!
        if (!window->isInModalLoop()) {
            // Normal case: window was resized outside of drag
            pendingResize.store(true, std::memory_order_release);
        }
        // During modal loop: render thread keeps running independently
        // No action needed - this is the whole point of the render thread!
        });

    // Create default scene
    createDefaultScene();

    // Setup UI (must be after render thread starts)
    setupUI();

    printControls();

    std::cout << "=== Initialization Complete ===\n" << std::endl;
}

void Application::setupUI() {
    std::cout << "[DEBUG] Setting up UI..." << std::endl;

    using namespace libre::ui;

    // Note: UIManager will need modification to work with render thread
    // For now, we initialize it but may need to adjust how it renders
    uiManager = std::make_unique<UIManager>();

    // UI initialization needs to happen differently with render thread
    // TODO: UIManager needs to be refactored to submit draw commands to render thread
    // For now, we'll initialize it minimally
    // uiManager->init(vulkanContext.get(), swapChain->getRenderPass(), window->getHandle());

    // Setup callback for when window moves to different monitor (DPI change)
    glfwSetWindowContentScaleCallback(window->getHandle(),
        [](GLFWwindow* win, float xscale, float yscale) {
            libre::ui::UIScale::instance().onMonitorChanged(win);
        }
    );

    std::cout << "[DEBUG] UI setup complete (render thread mode)" << std::endl;
}

void Application::cleanup() {
    std::cout << "\n=== Cleaning Up ===" << std::endl;

    // Stop render thread first (this waits for GPU to finish)
    if (renderThread) {
        renderThread->stop();
        renderThread.reset();
    }

    // Now safe to clean up other resources
    uiManager.reset();
    camera.reset();
    inputManager.reset();
    window.reset();

    std::cout << "=== Cleanup Complete ===" << std::endl;
}

void Application::createDefaultScene() {
    std::cout << "[DEBUG] Creating default scene..." << std::endl;

    auto& world = libre::Editor::instance().getWorld();

    // Create a cube
    auto cube = libre::Primitives::createCube(world, 2.0f, "DefaultCube");

    // Create a sphere
    auto sphere = libre::Primitives::createSphere(world, 1.0f, 32, 16, "Sphere");
    if (auto* t = sphere.get<libre::TransformComponent>()) {
        t->position = glm::vec3(3.0f, 0.0f, 0.0f);
        t->dirty = true;
    }

    // Create a cylinder
    auto cylinder = libre::Primitives::createCylinder(world, 0.5f, 2.0f, 32, "Cylinder");
    if (auto* t = cylinder.get<libre::TransformComponent>()) {
        t->position = glm::vec3(-3.0f, 0.0f, 0.0f);
        t->dirty = true;
    }

    std::cout << "[OK] Default scene created with "
        << world.getEntityCount() << " entities" << std::endl;
}

void Application::printControls() {
    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "Middle Mouse: Orbit camera" << std::endl;
    std::cout << "Shift + Middle Mouse: Pan camera" << std::endl;
    std::cout << "Scroll: Zoom" << std::endl;
    std::cout << "Left Click: Select object" << std::endl;
    std::cout << "F11: Toggle fullscreen" << std::endl;
    std::cout << "ESC: Exit" << std::endl;
    std::cout << "================\n" << std::endl;
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void Application::mainLoop() {
    std::cout << "[MainLoop] Starting main loop (render thread architecture)" << std::endl;

    while (!window->shouldClose()) {
        // ====================================================================
        // 1. POLL EVENTS (Fast - never blocks)
        // ====================================================================
        window->pollEvents();

        // ====================================================================
        // 2. HANDLE RESIZE COMPLETION
        // ====================================================================
        // Only request swapchain recreation when resize is COMPLETE
        // (not during the resize drag)
        if (!window->isInModalLoop() &&
            (pendingResize.load(std::memory_order_acquire) || window->wasResized())) {

            int w, h;
            glfwGetFramebufferSize(window->getHandle(), &w, &h);

            if (w > 0 && h > 0) {
                // Update camera aspect ratio
                camera->setAspectRatio(static_cast<float>(w) / static_cast<float>(h));

                // Request swapchain recreation (async - render thread handles it)
                renderThread->requestSwapchainRecreate(
                    static_cast<uint32_t>(w),
                    static_cast<uint32_t>(h)
                );

                std::cout << "[MainLoop] Resize complete: " << w << "x" << h << std::endl;
            }

            window->resetResizeFlag();
            pendingResize.store(false, std::memory_order_release);
        }

        // ====================================================================
        // 3. SKIP IF MINIMIZED
        // ====================================================================
        if (isMinimized()) {
            // Sleep to avoid spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        // ====================================================================
        // 4. CALCULATE DELTA TIME
        // ====================================================================
        auto currentTime = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        totalTime = std::chrono::duration<float>(currentTime - startTime).count();
        fps = 1.0f / deltaTime;

        // ====================================================================
        // 5. PROCESS INPUT
        // ====================================================================
        processInput(deltaTime);

        // ====================================================================
        // 6. UPDATE GAME STATE
        // ====================================================================
        update(deltaTime);

        // ====================================================================
        // 7. PREPARE FRAME DATA
        // ====================================================================
        libre::FrameData frameData = prepareFrameData();

        // ====================================================================
        // 8. SUBMIT TO RENDER THREAD (Non-blocking!)
        // ====================================================================
        renderThread->submitFrameData(frameData);

        // ====================================================================
        // 9. UPDATE INPUT STATE
        // ====================================================================
        inputManager->update();

        // ====================================================================
        // 10. FRAME COMPLETE - Loop continues immediately!
        // ====================================================================
        // Main thread NEVER waits for GPU. This is the key to smooth window drag.
    }

    std::cout << "[MainLoop] Main loop ended" << std::endl;
}

// ============================================================================
// PREPARE FRAME DATA
// ============================================================================

libre::FrameData Application::prepareFrameData() {
    libre::FrameData data;

    // Frame info
    data.frameNumber = frameNumber++;
    data.deltaTime = deltaTime;
    data.totalTime = totalTime;

    // Camera
    data.camera.viewMatrix = camera->getViewMatrix();
    data.camera.projectionMatrix = camera->getProjectionMatrix();
    data.camera.position = camera->getPosition();
    data.camera.forward = glm::normalize(camera->getTarget() - camera->getPosition());
    data.camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    data.camera.fov = camera->fov;
    data.camera.nearPlane = camera->nearPlane;
    data.camera.farPlane = camera->farPlane;

    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);
    data.camera.aspectRatio = static_cast<float>(w) / static_cast<float>(std::max(1, h));

    // Lighting
    data.light.direction = glm::normalize(glm::vec3(0.5f, 0.7f, 0.5f));
    data.light.color = glm::vec3(1.0f);
    data.light.intensity = 1.0f;
    data.light.ambientStrength = 0.15f;

    // Viewport
    data.viewport.width = static_cast<uint32_t>(std::max(1, w));
    data.viewport.height = static_cast<uint32_t>(std::max(1, h));
    data.viewport.showGrid = true;

    // Collect meshes from ECS
    auto& editor = libre::Editor::instance();
    auto& world = editor.getWorld();

    world.forEach<libre::MeshComponent>([&](libre::EntityID id, libre::MeshComponent& mesh) {
        auto* transform = world.getComponent<libre::TransformComponent>(id);
        if (transform) {
            libre::RenderableMesh rm;
            rm.meshHandle = static_cast<libre::MeshHandle>(id);  // Temporary: use entity ID
            rm.modelMatrix = transform->worldMatrix;  // Direct access to worldMatrix
            rm.entityId = id;
            rm.isSelected = editor.isSelected(id);  // Use Editor's isSelected
            rm.color = rm.isSelected ? glm::vec4(1.0f, 0.5f, 0.0f, 1.0f)
                : glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
            data.meshes.push_back(rm);
        }
        });

    // UI
    data.ui.screenWidth = static_cast<float>(data.viewport.width);
    data.ui.screenHeight = static_cast<float>(data.viewport.height);
    data.ui.dpiScale = 1.0f;  // TODO: Get from UIScale

    return data;
}

// ============================================================================
// INPUT PROCESSING
// ============================================================================

void Application::processInput(float dt) {
    auto& editor = libre::Editor::instance();

    // Exit
    if (inputManager->isKeyPressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window->getHandle(), GLFW_TRUE);
    }

    // Toggle fullscreen with F11
    if (inputManager->isKeyJustPressed(GLFW_KEY_F11)) {
        static bool isFullscreen = false;
        static int savedX, savedY, savedWidth, savedHeight;

        if (!isFullscreen) {
            glfwGetWindowPos(window->getHandle(), &savedX, &savedY);
            glfwGetWindowSize(window->getHandle(), &savedWidth, &savedHeight);

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window->getHandle(), monitor, 0, 0,
                mode->width, mode->height, mode->refreshRate);
        }
        else {
            glfwSetWindowMonitor(window->getHandle(), nullptr,
                savedX, savedY, savedWidth, savedHeight, 0);
        }
        isFullscreen = !isFullscreen;
    }

    // Track modifier keys
    shiftHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
    ctrlHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_CONTROL);
    altHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_ALT) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_ALT);

    // Camera controls - use correct API
    double mouseX = inputManager->getMouseX();
    double mouseY = inputManager->getMouseY();

    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_MIDDLE)) {
        middleMouseDown = true;
        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    if (inputManager->isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_MIDDLE)) {
        middleMouseDown = false;
    }

    if (middleMouseDown) {
        double deltaX = mouseX - lastMouseX;
        double deltaY = mouseY - lastMouseY;

        if (shiftHeld) {
            camera->pan(static_cast<float>(deltaX) * 0.01f,
                static_cast<float>(-deltaY) * 0.01f);
        }
        else {
            camera->orbit(static_cast<float>(deltaX) * 0.5f,
                static_cast<float>(deltaY) * 0.5f);
        }

        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    // Zoom with scroll - use correct API
    double scrollY = inputManager->getScrollY();
    if (scrollY != 0.0) {
        camera->zoom(static_cast<float>(scrollY) * 0.5f);
    }

    // Selection with left click
    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        handleSelection();
    }
}

// ============================================================================
// UPDATE
// ============================================================================

void Application::update(float dt) {
    // Update ECS transforms
    updateTransforms();
}

void Application::updateTransforms() {
    auto& world = libre::Editor::instance().getWorld();

    world.forEach<libre::TransformComponent>([&](libre::EntityID id, libre::TransformComponent& t) {
        if (t.dirty) {
            if (world.getParent(id) == libre::INVALID_ENTITY) {
                t.worldMatrix = t.getLocalMatrix();
            }
            else {
                auto* parentT = world.getComponent<libre::TransformComponent>(world.getParent(id));
                if (parentT) {
                    t.worldMatrix = parentT->worldMatrix * t.getLocalMatrix();
                }
            }

            auto* bounds = world.getComponent<libre::BoundsComponent>(id);
            if (bounds) {
                bounds->updateWorldBounds(t.worldMatrix);
            }

            t.dirty = false;
        }
        });
}

void Application::syncECSToRenderer() {
    // TODO: This needs to be reimplemented for the render thread architecture
    // Mesh registration/unregistration should go through RenderThread
}

void Application::handleSelection() {
    // TODO: Implement GPU picking through render thread
    // For now, selection is disabled
}

void Application::openPreferencesWindow() {
    // TODO: Implement with UI system
}

// ============================================================================
// HELPERS
// ============================================================================

bool Application::isMinimized() const {
    int width, height;
    glfwGetFramebufferSize(window->getHandle(), &width, &height);
    return width == 0 || height == 0;
}