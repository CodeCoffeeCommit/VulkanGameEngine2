// src/core/Application.cpp
// COMPLETE FILE - Fixed version with correct API usage

#include "Application.h"
#include "Editor.h"
#include "FrameData.h"
#include "../render/RenderThread.h"
#include "../render/VulkanContext.h"  // Need full definition for vkDeviceWaitIdle
#include "../render/Mesh.h"
#include "../world/Primitives.h"
#include "../components/CoreComponents.h"  // For MeshComponent, TransformComponent, etc.
#include "../ui/UI.h"
#include "../ui/Widgets.h"
#include "../ui/UIScale.h"
#include <unordered_set>

#include <iostream>
#include <algorithm>
#include <thread>

// Window settings
static constexpr int WINDOW_WIDTH = 1600;
static constexpr int WINDOW_HEIGHT = 900;
static constexpr const char* WINDOW_TITLE = "LibreDCC - 3D Viewport";

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

    // Create input manager
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

    // FIX: Update transforms IMMEDIATELY after creating scene
    // This ensures worldMatrix is computed before first frame
    updateTransforms();

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

    uiManager = std::make_unique<UIManager>();

    // Wait for render thread to fully initialize Vulkan
    std::cout << "[DEBUG] Waiting for render thread to initialize Vulkan..." << std::endl;

    auto waitStart = std::chrono::steady_clock::now();
    while (!renderThread->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (std::chrono::steady_clock::now() - waitStart > std::chrono::seconds(10)) {
            std::cerr << "[ERROR] Timeout waiting for render thread!" << std::endl;
            return;
        }
    }

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

    // FIXED: Ensure GPU is idle before UI initialization (font atlas upload)
    vkDeviceWaitIdle(ctx->getDevice());

    uiManager->init(ctx, renderPass, window->getHandle());

    // Wait again after UI init
    vkDeviceWaitIdle(ctx->getDevice());

    // Connect UI render callback to render thread
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

    // ========================================================================
    // CREATE MENU BAR - FIXED: Use correct API
    // ========================================================================
    auto menuBar = std::make_unique<MenuBar>();

    // File Menu
    menuBar->addMenu("File", {
        MenuItem::Action("New", []() { std::cout << "New project\n"; }, "Ctrl+N"),
        MenuItem::Action("Open...", []() { std::cout << "Open project\n"; }, "Ctrl+O"),
        MenuItem::Action("Save", []() { std::cout << "Save project\n"; }, "Ctrl+S"),
        MenuItem::Action("Save As...", []() { std::cout << "Save As\n"; }, "Ctrl+Shift+S"),
        MenuItem::Separator(),
        MenuItem::Action("Import...", []() { std::cout << "Import\n"; }),
        MenuItem::Action("Export...", []() { std::cout << "Export\n"; }),
        MenuItem::Separator(),
        MenuItem::Action("Exit", [this]() { window->getHandle(); glfwSetWindowShouldClose(window->getHandle(), GLFW_TRUE); }, "Alt+F4")
        });

    // Edit Menu
    menuBar->addMenu("Edit", {
        MenuItem::Action("Undo", []() { std::cout << "Undo\n"; }, "Ctrl+Z"),
        MenuItem::Action("Redo", []() { std::cout << "Redo\n"; }, "Ctrl+Y"),
        MenuItem::Separator(),
        MenuItem::Action("Cut", []() { std::cout << "Cut\n"; }, "Ctrl+X"),
        MenuItem::Action("Copy", []() { std::cout << "Copy\n"; }, "Ctrl+C"),
        MenuItem::Action("Paste", []() { std::cout << "Paste\n"; }, "Ctrl+V"),
        MenuItem::Separator(),
        MenuItem::Action("Preferences...", []() { std::cout << "Preferences\n"; })
        });

    // View Menu - FIXED: Use correct Camera methods
    menuBar->addMenu("View", {
        MenuItem::Toggle("Show Grid", &showGrid, "G"),
        MenuItem::Toggle("Show Wireframe", &showWireframe, "Z"),
        MenuItem::Separator(),
        MenuItem::Action("Reset View", [this]() {
            if (camera) camera->reset();
            std::cout << "View reset\n";
        }, "Home"),
        MenuItem::Separator(),
        MenuItem::Action("Front", [this]() { if (camera) camera->setFront(); }, "Numpad 1"),
        MenuItem::Action("Right", [this]() { if (camera) camera->setRight(); }, "Numpad 3"),
        MenuItem::Action("Top", [this]() { if (camera) camera->setTop(); }, "Numpad 7")
        });

    // FIXED: Use setMenuBar() instead of non-existent createWidget()
    uiManager->setMenuBar(std::move(menuBar));

    // Initial layout
    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);
    uiManager->layout(static_cast<float>(w), static_cast<float>(h));

    // Setup DPI change callback
    glfwSetWindowContentScaleCallback(window->getHandle(),
        [](GLFWwindow* win, float xscale, float yscale) {
            std::cout << "[UI] Content scale changed: " << xscale << ", " << yscale << std::endl;
            libre::ui::UIScale::instance().onMonitorChanged(win);
        });

    std::cout << "[DEBUG] UI setup complete" << std::endl;
}

// ============================================================================
// CLEANUP
// ============================================================================

void Application::cleanup() {
    std::cout << "\n=== Cleaning up Application ===" << std::endl;

    if (renderThread) {
        renderThread->stop();
        renderThread.reset();
    }

    if (uiManager) {
        uiManager->cleanup();
        uiManager.reset();
    }

    camera.reset();
    inputManager.reset();
    window.reset();

    std::cout << "=== Cleanup Complete ===" << std::endl;
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void Application::mainLoop() {
    std::cout << "[MainLoop] Starting main loop (render thread architecture)" << std::endl;

    // Track which entities have pending uploads
    std::unordered_set<libre::EntityID> pendingUploads;
    uint64_t lastProcessedFrame = 0;

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
        // 7. CHECK IF RENDER THREAD HAS PROCESSED OUR UPLOADS
        // ====================================================================
        uint64_t lastCompleted = renderThread->getLastCompletedFrame();
        if (lastCompleted > lastProcessedFrame && !pendingUploads.empty()) {
            // Render thread processed our frame - safe to clear gpuDirty
            auto& world = libre::Editor::instance().getWorld();
            for (libre::EntityID id : pendingUploads) {
                if (auto* mesh = world.getComponent<libre::MeshComponent>(id)) {
                    mesh->gpuDirty = false;
                    if (lastProcessedFrame <= 10) {
                        std::cout << "[MainLoop] Confirmed upload for entity " << id << std::endl;
                    }
                }
            }
            pendingUploads.clear();
            lastProcessedFrame = lastCompleted;
        }

        // ====================================================================
        // 8. PREPARE FRAME DATA
        // ====================================================================
        libre::FrameData frameData = prepareFrameData();

        // ====================================================================
        // 9. TRACK ENTITIES WITH PENDING UPLOADS
        // ====================================================================
        for (const auto& upload : frameData.meshUploads) {
            pendingUploads.insert(upload.entityId);
        }

        // ====================================================================
        // 10. SUBMIT TO RENDER THREAD (Non-blocking!)
        // ====================================================================
        renderThread->submitFrameData(frameData);

        // ====================================================================
        // 11. UPDATE INPUT STATE
        // ====================================================================
        inputManager->update();
    }

    std::cout << "[MainLoop] Main loop ended" << std::endl;
}

// ============================================================================
// PROCESS INPUT - FIXED: Use correct UIManager methods
// ============================================================================

void Application::processInput(float dt) {
    double mouseX = inputManager->getMouseX();
    double mouseY = inputManager->getMouseY();

    // Forward mouse events to UI using correct method names
    if (uiManager) {
        uiManager->onMouseMove(static_cast<float>(mouseX), static_cast<float>(mouseY));
    }

    // Left mouse button
    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (uiManager) {
            uiManager->onMouseButton(libre::ui::MouseButton::Left, true);
        }
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

    // Camera controls (only when UI is not capturing input)
    // REMOVED: isMouseOverUI() doesn't exist - we'll handle this differently
    handleCameraInput(dt, mouseX, mouseY);

    // Handle keyboard shortcuts
    handleKeyboardShortcuts();
}

// ============================================================================
// CAMERA INPUT HANDLING
// ============================================================================

void Application::handleCameraInput(float dt, double mouseX, double mouseY) {
    if (!camera) return;

    // Track middle mouse button state
    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_MIDDLE)) {
        middleMouseDown = true;
        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    if (inputManager->isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_MIDDLE)) {
        middleMouseDown = false;
    }

    // Middle mouse button for orbit/pan
    if (middleMouseDown) {
        double dx = mouseX - lastMouseX;
        double dy = mouseY - lastMouseY;

        if (shiftHeld) {
            // Pan
            camera->pan(static_cast<float>(dx) * 0.01f, static_cast<float>(-dy) * 0.01f);
        }
        else {
            // Orbit
            camera->orbit(static_cast<float>(dx) * 0.5f, static_cast<float>(dy) * 0.5f);
        }

        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    // Scroll wheel for zoom - FIXED: Use getScrollY() not getScrollDelta()
    double scrollY = inputManager->getScrollY();
    if (scrollY != 0.0) {
        camera->zoom(static_cast<float>(scrollY) * 0.5f);

        // Also forward to UI
        if (uiManager) {
            uiManager->onMouseScroll(static_cast<float>(scrollY));
        }
    }
}

// ============================================================================
// KEYBOARD SHORTCUTS - FIXED: Use correct Camera methods
// ============================================================================

void Application::handleKeyboardShortcuts() {
    // Toggle grid
    if (inputManager->isKeyJustPressed(GLFW_KEY_G)) {
        showGrid = !showGrid;
        std::cout << "Grid: " << (showGrid ? "ON" : "OFF") << std::endl;
    }

    // Toggle wireframe
    if (inputManager->isKeyJustPressed(GLFW_KEY_Z)) {
        showWireframe = !showWireframe;
        std::cout << "Wireframe: " << (showWireframe ? "ON" : "OFF") << std::endl;
    }

    // Reset view
    if (inputManager->isKeyJustPressed(GLFW_KEY_HOME)) {
        if (camera) camera->reset();
    }

    // View presets with numpad - FIXED: Use setFront/setRight/setTop methods
    if (inputManager->isKeyJustPressed(GLFW_KEY_KP_1)) {
        if (camera) camera->setFront();
    }
    if (inputManager->isKeyJustPressed(GLFW_KEY_KP_3)) {
        if (camera) camera->setRight();
    }
    if (inputManager->isKeyJustPressed(GLFW_KEY_KP_7)) {
        if (camera) camera->setTop();
    }

    // Forward key events to UI
    if (uiManager) {
        // Forward escape key
        if (inputManager->isKeyJustPressed(GLFW_KEY_ESCAPE)) {
            uiManager->onKey(GLFW_KEY_ESCAPE, true, shiftHeld, ctrlHeld, altHeld);
        }
    }
}

// ============================================================================
// UPDATE
// ============================================================================

void Application::update(float dt) {
    updateTransforms();
}

// ============================================================================
// UPDATE TRANSFORMS
// ============================================================================

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
// PREPARE FRAME DATA - FIXED: Use correct World API
// ============================================================================

libre::FrameData Application::prepareFrameData() {
    libre::FrameData data;
    static uint64_t frameCounter = 0;
    data.frameNumber = ++frameCounter;
    data.deltaTime = deltaTime;
    data.totalTime = totalTime;

    int fw, fh;
    glfwGetFramebufferSize(window->getHandle(), &fw, &fh);

    // Camera data
    if (camera) {
        data.camera.viewMatrix = camera->getViewMatrix();
        data.camera.projectionMatrix = camera->getProjectionMatrix();
        data.camera.position = camera->getPosition();
        data.camera.fov = camera->fov;
        data.camera.nearPlane = camera->nearPlane;
        data.camera.farPlane = camera->farPlane;
        data.camera.aspectRatio = static_cast<float>(fw) / static_cast<float>(std::max(1, fh));
    }

    // Lighting
    data.light.direction = glm::vec4(glm::normalize(glm::vec3(0.5f, 0.7f, 0.5f)), 0.0f);
    data.light.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    data.light.intensity = 1.0f;
    data.light.ambientStrength = 0.15f;

    // Viewport
    data.viewport.width = static_cast<uint32_t>(std::max(1, fw));
    data.viewport.height = static_cast<uint32_t>(std::max(1, fh));
    data.viewport.showGrid = showGrid;
    data.wireframeMode = showWireframe;

    // UI
    data.ui.screenWidth = static_cast<float>(fw);
    data.ui.screenHeight = static_cast<float>(fh);

    // ========================================================================
    // COLLECT MESHES FROM ECS
    // ========================================================================
    auto& editor = libre::Editor::instance();
    auto& world = editor.getWorld();


    // Get last completed frame to check if uploads were processed
    uint64_t lastCompletedFrame = renderThread->getLastCompletedFrame();

    // Diagnostic counters
    size_t totalMeshComponents = 0;
    size_t meshesNeedingUpload = 0;

    // Iterate over all entities with MeshComponent
    world.forEach<libre::MeshComponent>([&](libre::EntityID id, libre::MeshComponent& meshComp) {
        totalMeshComponents++;

        auto* transform = world.getComponent<libre::TransformComponent>(id);
        auto* render = world.getComponent<libre::RenderComponent>(id);

        if (!transform) return;
        if (render && !render->visible) return;

        // Diagnostic logging (first 10 frames)
        if (data.frameNumber <= 10) {
            std::cout << "[prepareFrameData] Entity " << id
                << " | gpuDirty=" << (meshComp.gpuDirty ? "TRUE" : "false")
                << " | vertices=" << meshComp.vertices.size()
                << " | indices=" << meshComp.indices.size()
                << " | worldMatrix[3]=" << transform->worldMatrix[3][0] << ","
                << transform->worldMatrix[3][1] << "," << transform->worldMatrix[3][2]
                << std::endl;
        }



        // If mesh needs GPU upload
        if (meshComp.gpuDirty && !meshComp.vertices.empty()) {
            meshesNeedingUpload++;

            libre::MeshUploadData upload;
            upload.entityId = id;
            upload.vertices.reserve(meshComp.vertices.size());

            // Convert MeshVertex to UploadVertex
            for (const auto& v : meshComp.vertices) {
                libre::UploadVertex uv;
                uv.position = v.position;
                uv.normal = v.normal;
                uv.color = v.color;
                upload.vertices.push_back(uv);
            }
            upload.indices = meshComp.indices;

            data.meshUploads.push_back(std::move(upload));

            // Mark as uploaded (will be set to false by render thread)
            //remove -->  meshComp.gpuDirty = false;

            if (data.frameNumber <= 5) {
                std::cout << "[prepareFrameData] >>> QUEUED upload for entity "
                    << id << " (" << meshComp.vertices.size() << " verts, "
                    << meshComp.indices.size() << " indices)" << std::endl;
            }
        }

        // Add to render list
        libre::RenderableMesh rm;
        rm.meshHandle = static_cast<libre::MeshHandle>(id);
        rm.modelMatrix = transform->worldMatrix;
        rm.entityId = id;
        rm.isSelected = editor.isSelected(id);

        // Get color from render component or use default
        if (render) {
            rm.color = glm::vec4(render->baseColor, render->opacity);
        }
        else {
            rm.color = rm.isSelected ?
                glm::vec4(1.0f, 0.6f, 0.2f, 1.0f) :  // Orange when selected
                glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);   // Default gray
        }

        data.meshes.push_back(rm);
        });

    // Diagnostic: Print frame summary (first 5 frames only)
    if (data.frameNumber <= 5) {
        std::cout << "[prepareFrameData] Frame " << data.frameNumber
            << " | MeshComponents: " << totalMeshComponents
            << " | NeedUpload: " << meshesNeedingUpload
            << " | Renderables: " << data.meshes.size() << std::endl;
    }

    return data;
}

// ============================================================================
// SELECTION HANDLING
// ============================================================================

void Application::handleSelection() {
    // Selection logic here
}

// ============================================================================
// IS MINIMIZED
// ============================================================================

bool Application::isMinimized() const {
    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);
    return (w == 0 || h == 0);
}

// ============================================================================
// CREATE DEFAULT SCENE - FIXED: Use correct Primitives API
// ============================================================================

void Application::createDefaultScene() {
    std::cout << "[DEBUG] Creating default scene..." << std::endl;

    auto& world = libre::Editor::instance().getWorld();

    // Create a cube
    auto cube = libre::Primitives::createCube(world, 2.0f, "DefaultCube");

    // Create a sphere at a different position
    auto sphere = libre::Primitives::createSphere(world, 1.0f, 32, 16, "Sphere");
    if (auto* t = sphere.get<libre::TransformComponent>()) {
        t->position = glm::vec3(3.0f, 0.0f, 0.0f);
        t->dirty = true;
    }

    // Create a cylinder at a different position
    auto cylinder = libre::Primitives::createCylinder(world, 0.5f, 2.0f, 32, "Cylinder");
    if (auto* t = cylinder.get<libre::TransformComponent>()) {
        t->position = glm::vec3(-3.0f, 0.0f, 0.0f);
        t->dirty = true;
    }

    std::cout << "[OK] Default scene created with "
        << world.getEntityCount() << " entities" << std::endl;
}

// ============================================================================
// PRINT CONTROLS
// ============================================================================

void Application::printControls() {
    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "Middle Mouse: Orbit camera" << std::endl;
    std::cout << "Shift + Middle Mouse: Pan camera" << std::endl;
    std::cout << "Scroll Wheel: Zoom" << std::endl;
    std::cout << "G: Toggle grid" << std::endl;
    std::cout << "Z: Toggle wireframe" << std::endl;
    std::cout << "Home: Reset view" << std::endl;
    std::cout << "Numpad 1/3/7: Front/Right/Top view" << std::endl;
    std::cout << "Ctrl+Numpad: Opposite views" << std::endl;
    std::cout << "================\n" << std::endl;
}