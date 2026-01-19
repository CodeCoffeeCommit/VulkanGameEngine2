// src/core/Application.cpp
//
// NEW ARCHITECTURE: Main thread never blocks on GPU.
// All rendering happens on dedicated RenderThread.
//

#include "Application.h"
#include "Editor.h"
#include "FrameData.h"
#include "Selection.h"
#include "../render/RenderThread.h"
#include "../render/Mesh.h"
#include "../world/Primitives.h"
#include "../ui/UI.h"
#include "../ui/Widgets.h"
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
        if (!window->isInModalLoop()) {
            pendingResize.store(true, std::memory_order_release);
        }
        });

    // Create default scene
    createDefaultScene();

    // Setup UI (must be after render thread starts and Vulkan is initialized)
    setupUI();

    printControls();

    std::cout << "=== Initialization Complete ===\n" << std::endl;
}

// ============================================================================
// UI SETUP - FIXED VERSION
// ============================================================================

void Application::setupUI() {
    std::cout << "[DEBUG] Setting up UI..." << std::endl;

    using namespace libre::ui;

    // Create UIManager
    uiManager = std::make_unique<UIManager>();

    // =========================================================================
    // CRITICAL: Wait for render thread to fully initialize Vulkan
    // =========================================================================
    std::cout << "[DEBUG] Waiting for render thread to initialize Vulkan..." << std::endl;

    auto waitStart = std::chrono::steady_clock::now();
    while (!renderThread->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (std::chrono::steady_clock::now() - waitStart > std::chrono::seconds(10)) {
            std::cerr << "[ERROR] Timeout waiting for render thread!" << std::endl;
            return;
        }
    }

    // Get Vulkan objects from render thread
    VulkanContext* ctx = renderThread->getVulkanContext();
    VkRenderPass renderPass = renderThread->getRenderPass();

    if (!ctx) {
        std::cerr << "[ERROR] VulkanContext is null!" << std::endl;
        return;
    }

    if (renderPass == VK_NULL_HANDLE) {
        std::cerr << "[ERROR] RenderPass is null!" << std::endl;
        return;
    }

    std::cout << "[DEBUG] Got Vulkan objects from render thread" << std::endl;

    // =========================================================================
    // INITIALIZE UI MANAGER
    // =========================================================================
    uiManager->init(ctx, renderPass, window->getHandle());
    std::cout << "[DEBUG] UIManager initialized" << std::endl;

    // =========================================================================
    // CREATE MENU BAR
    // =========================================================================
    auto menuBar = std::make_unique<MenuBar>();

    // File menu
    menuBar->addMenu("File", {
        MenuItem::Action("New", []() {
            std::cout << "[Menu] New file\n";
        }, "Ctrl+N"),
        MenuItem::Action("Open...", []() {
            std::cout << "[Menu] Open file\n";
        }, "Ctrl+O"),
        MenuItem::Action("Save", []() {
            std::cout << "[Menu] Save file\n";
        }, "Ctrl+S"),
        MenuItem::Action("Save As...", []() {
            std::cout << "[Menu] Save As\n";
        }, "Ctrl+Shift+S"),
        MenuItem::Separator(),
        MenuItem::Action("Exit", [this]() {
            std::cout << "[Menu] Exit\n";
            glfwSetWindowShouldClose(window->getHandle(), GLFW_TRUE);
        }, "Alt+F4")
        });

    // Edit menu
    menuBar->addMenu("Edit", {
        MenuItem::Action("Undo", []() {
            libre::Editor::instance().undo();
        }, "Ctrl+Z"),
        MenuItem::Action("Redo", []() {
            libre::Editor::instance().redo();
        }, "Ctrl+Y"),
        MenuItem::Separator(),
        MenuItem::Action("Select All", []() {
            libre::Editor::instance().selectAll();
        }, "Ctrl+A"),
        MenuItem::Action("Deselect All", []() {
            libre::Editor::instance().deselectAll();
        })
        });

    // View menu
    menuBar->addMenu("View", {
        MenuItem::Action("Reset View", [this]() {
            std::cout << "[Menu] Reset View\n";
            camera->reset();
        }),
        MenuItem::Action("Front View", [this]() {
            camera->setFront();
        }, "Numpad 1"),
        MenuItem::Action("Right View", [this]() {
            camera->setRight();
        }, "Numpad 3"),
        MenuItem::Action("Top View", [this]() {
            camera->setTop();
        }, "Numpad 7")
        });

    // Help menu
    menuBar->addMenu("Help", {
        MenuItem::Action("About", []() {
            std::cout << "=== LibreDCC ===" << std::endl;
            std::cout << "Version: 0.1.0 (Alpha)" << std::endl;
            std::cout << "A modular 3D creative suite" << std::endl;
        })
        });

    uiManager->setMenuBar(std::move(menuBar));
    std::cout << "[DEBUG] Menu bar created" << std::endl;

    // =========================================================================
    // CONNECT UI RENDERING TO RENDER THREAD
    // =========================================================================
    renderThread->setUIRenderCallback([this](void* commandBuffer) {
        if (uiManager) {
            uint32_t w, h;
            renderThread->getSwapchainExtent(w, h);

            if (w > 0 && h > 0) {
                uiManager->layout(static_cast<float>(w), static_cast<float>(h));
                uiManager->render(static_cast<VkCommandBuffer>(commandBuffer));
            }
        }
        });
    std::cout << "[DEBUG] UI render callback connected" << std::endl;

    // =========================================================================
    // INITIAL LAYOUT
    // =========================================================================
    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);
    uiManager->layout(static_cast<float>(w), static_cast<float>(h));

    // =========================================================================
    // SETUP DPI CHANGE CALLBACK
    // =========================================================================
    glfwSetWindowContentScaleCallback(window->getHandle(),
        [](GLFWwindow* win, float xscale, float yscale) {
            std::cout << "[UI] Content scale changed: " << xscale << ", " << yscale << std::endl;
            libre::ui::UIScale::instance().onMonitorChanged(win);
        }
    );

    std::cout << "[DEBUG] UI setup complete" << std::endl;
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
        if (!window->isInModalLoop() &&
            (pendingResize.load(std::memory_order_acquire) || window->wasResized())) {

            int w, h;
            glfwGetFramebufferSize(window->getHandle(), &w, &h);

            if (w > 0 && h > 0) {
                camera->setAspectRatio(static_cast<float>(w) / static_cast<float>(h));

                renderThread->requestSwapchainRecreate(
                    static_cast<uint32_t>(w),
                    static_cast<uint32_t>(h)
                );

                if (uiManager) {
                    uiManager->layout(static_cast<float>(w), static_cast<float>(h));
                }

                std::cout << "[MainLoop] Resize complete: " << w << "x" << h << std::endl;
            }

            window->resetResizeFlag();
            pendingResize.store(false, std::memory_order_release);
        }

        // ====================================================================
        // 3. SKIP IF MINIMIZED
        // ====================================================================
        if (isMinimized()) {
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

        // ====================================================================
        // 5. PROCESS INPUT
        // ====================================================================
        processInput(deltaTime);

        // ====================================================================
        // 6. UPDATE GAME/EDITOR STATE
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
    }

    std::cout << "[MainLoop] Main loop ended" << std::endl;
}

// ============================================================================
// PROCESS INPUT
// ============================================================================

void Application::processInput(float dt) {
    double mouseX = inputManager->getMouseX();
    double mouseY = inputManager->getMouseY();

    // Forward mouse events to UI
    if (uiManager) {
        uiManager->onMouseMove(static_cast<float>(mouseX), static_cast<float>(mouseY));
    }

    // Left mouse button
    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (uiManager) {
            uiManager->onMouseButton(libre::ui::MouseButton::Left, true);
        }
        handleSelection();
    }

    if (inputManager->isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
        if (uiManager) {
            uiManager->onMouseButton(libre::ui::MouseButton::Left, false);
        }
    }

    // Right mouse button
    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        if (uiManager) {
            uiManager->onMouseButton(libre::ui::MouseButton::Right, true);
        }
    }

    if (inputManager->isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
        if (uiManager) {
            uiManager->onMouseButton(libre::ui::MouseButton::Right, false);
        }
    }

    // Modifier keys
    shiftHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
    ctrlHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_CONTROL);
    altHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_ALT) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_ALT);

    // =========================================================================
    // CAMERA CONTROLS - MIDDLE MOUSE BUTTON
    // =========================================================================
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

    // Scroll wheel - zoom
    double scrollY = inputManager->getScrollY();
    if (scrollY != 0.0) {
        if (uiManager) {
            uiManager->onMouseScroll(static_cast<float>(scrollY));
        }
        camera->zoom(static_cast<float>(scrollY) * 0.5f);
    }

    // Keyboard shortcuts
    if (inputManager->isKeyJustPressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window->getHandle(), GLFW_TRUE);
    }

    // Forward keyboard events to UI
    if (uiManager) {
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++) {
            if (inputManager->isKeyJustPressed(key)) {
                uiManager->onKey(key, true, shiftHeld, ctrlHeld, altHeld);
            }
            if (inputManager->isKeyJustReleased(key)) {
                uiManager->onKey(key, false, shiftHeld, ctrlHeld, altHeld);
            }
        }
    }
}

// ============================================================================
// UPDATE
// ============================================================================

void Application::update(float dt) {
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

    // Lighting - use vec4 for color (RGBA)
    data.light.direction = glm::normalize(glm::vec4(0.5f, 0.7f, 0.5f, 0.0f));
    data.light.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
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
            rm.meshHandle = static_cast<libre::MeshHandle>(id);
            rm.modelMatrix = transform->worldMatrix;
            rm.entityId = id;
            rm.isSelected = editor.isSelected(id);
            rm.color = rm.isSelected ? glm::vec4(1.0f, 0.5f, 0.0f, 1.0f) : glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
            data.meshes.push_back(rm);
        }
        });

    // UI data
    data.ui.screenWidth = static_cast<float>(w);
    data.ui.screenHeight = static_cast<float>(h);
    data.ui.dpiScale = libre::ui::UIScale::instance().getScaleFactor();

    return data;
}

// ============================================================================
// SELECTION
// ============================================================================

void Application::handleSelection() {
    double mouseX = inputManager->getMouseX();
    double mouseY = inputManager->getMouseY();

    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);

    libre::Ray ray = libre::SelectionSystem::screenToRay(
        *camera,
        static_cast<float>(mouseX),
        static_cast<float>(mouseY),
        w, h
    );

    auto& world = libre::Editor::instance().getWorld();
    libre::HitResult hit = libre::SelectionSystem::raycast(world, ray);

    auto& editor = libre::Editor::instance();

    if (hit.hit()) {
        if (!shiftHeld) {
            editor.deselectAll();
        }
        editor.select(hit.entity);
    }
    else if (!shiftHeld) {
        editor.deselectAll();
    }
}

// ============================================================================
// HELPERS
// ============================================================================

bool Application::isMinimized() const {
    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);
    return (w == 0 || h == 0);
}