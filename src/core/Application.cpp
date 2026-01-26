// src/core/Application.cpp
// COMPLETE FILE - Fixed initialization order to prevent Vulkan threading issues

#include "Application.h"
#include "Editor.h"
#include "FrameData.h"
#include "../render/RenderThread.h"
#include "../render/Mesh.h"
#include "../world/Primitives.h"
#include "../ui/UI.h"
#include "../ui/Widgets.h"
#include "../ui/UIScale.h"

#include <iostream>
#include <algorithm>

// Window settings
static constexpr int WINDOW_WIDTH = 1280;
static constexpr int WINDOW_HEIGHT = 720;
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
// INITIALIZATION - FIXED ORDER
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

    // ========================================================================
    // CRITICAL: Create render thread but DON'T start it yet
    // ========================================================================
    renderThread = std::make_unique<libre::RenderThread>();

    // Start render thread - this initializes Vulkan
    if (!renderThread->start(window.get())) {
        throw std::runtime_error("Failed to start render thread!");
    }

    // ========================================================================
    // CRITICAL: Setup UI AFTER render thread has initialized Vulkan
    // but BEFORE we start the main loop (while render thread is idle)
    // ========================================================================
    std::cout << "[DEBUG] Setting up UI (render thread paused)..." << std::endl;

    // Wait for any pending GPU work to complete before UI init
    if (renderThread->getVulkanContext()) {
        vkDeviceWaitIdle(renderThread->getVulkanContext()->getDevice());
    }

    setupUI();

    // Wait again after UI setup to ensure all uploads complete
    if (renderThread->getVulkanContext()) {
        vkDeviceWaitIdle(renderThread->getVulkanContext()->getDevice());
    }

    std::cout << "[DEBUG] UI setup complete" << std::endl;

    // ========================================================================
    // Create default scene AFTER everything is initialized
    // ========================================================================
    createDefaultScene();

    printControls();

    std::cout << "\n=== Initialization Complete ===\n" << std::endl;
}

void Application::setupUI() {
    using namespace libre::ui;

    std::cout << "[DEBUG] Waiting for render thread Vulkan context..." << std::endl;

    // Get Vulkan objects from render thread
    VulkanContext* vulkanContext = renderThread->getVulkanContext();
    VkRenderPass renderPass = renderThread->getRenderPass();

    if (!vulkanContext || renderPass == VK_NULL_HANDLE) {
        std::cerr << "[ERROR] Vulkan not ready for UI initialization!" << std::endl;
        return;
    }

    // Create UI manager
    uiManager = std::make_unique<UIManager>();
    uiManager->init(vulkanContext, renderPass, window->getHandle());

    // Create menu bar
    auto menuBar = uiManager->createWidget<MenuBar>("mainMenuBar");

    // File menu
    auto fileMenu = std::make_shared<Menu>("File");
    fileMenu->addItem("New", "Ctrl+N", []() { std::cout << "New file\n"; });
    fileMenu->addItem("Open...", "Ctrl+O", []() { std::cout << "Open file\n"; });
    fileMenu->addItem("Save", "Ctrl+S", []() { std::cout << "Save file\n"; });
    fileMenu->addSeparator();
    fileMenu->addItem("Exit", "Alt+F4", []() { std::cout << "Exit\n"; });
    menuBar->addMenu(fileMenu);

    // Edit menu
    auto editMenu = std::make_shared<Menu>("Edit");
    editMenu->addItem("Undo", "Ctrl+Z", []() { std::cout << "Undo\n"; });
    editMenu->addItem("Redo", "Ctrl+Y", []() { std::cout << "Redo\n"; });
    editMenu->addSeparator();
    editMenu->addItem("Cut", "Ctrl+X", []() { std::cout << "Cut\n"; });
    editMenu->addItem("Copy", "Ctrl+C", []() { std::cout << "Copy\n"; });
    editMenu->addItem("Paste", "Ctrl+V", []() { std::cout << "Paste\n"; });
    menuBar->addMenu(editMenu);

    // View menu
    auto viewMenu = std::make_shared<Menu>("View");
    viewMenu->addItem("Reset View", "Home", []() { std::cout << "Reset view\n"; });
    viewMenu->addItem("Frame Selected", "F", []() { std::cout << "Frame selected\n"; });
    viewMenu->addSeparator();
    viewMenu->addItem("Wireframe", "Z", []() { std::cout << "Toggle wireframe\n"; });
    viewMenu->addItem("Solid", "", []() { std::cout << "Solid mode\n"; });
    menuBar->addMenu(viewMenu);

    // Initial layout
    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);
    uiManager->layout(static_cast<float>(w), static_cast<float>(h));

    // Connect UI render callback to render thread
    renderThread->setUIRenderCallback(
        [this](void* commandBuffer) {
            if (uiManager) {
                uiManager->render(static_cast<VkCommandBuffer>(commandBuffer));
            }
        }
    );

    std::cout << "[DEBUG] UI render callback connected" << std::endl;
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
        // 2. CHECK FOR RESIZE COMPLETION
        // ====================================================================
        if (window->wasResized()) {
            int w, h;
            glfwGetFramebufferSize(window->getHandle(), &w, &h);

            if (w > 0 && h > 0) {
                std::cout << "[MainLoop] Resize complete: " << w << "x" << h << std::endl;
                renderThread->requestSwapchainRecreate(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
                camera->setAspectRatio(static_cast<float>(w) / static_cast<float>(h));

                if (uiManager) {
                    uiManager->layout(static_cast<float>(w), static_cast<float>(h));
                }
            }
            window->resetResizedFlag();
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
        auto now = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        lastFrameTime = now;
        totalTime = std::chrono::duration<float>(now - startTime).count();

        // ====================================================================
        // 5. PROCESS INPUT
        // ====================================================================
        processInput(deltaTime);

        // ====================================================================
        // 6. UPDATE
        // ====================================================================
        update(deltaTime);

        // ====================================================================
        // 7. UPDATE UI
        // ====================================================================
        if (uiManager) {
            float mx = static_cast<float>(inputManager->getMouseX());
            float my = static_cast<float>(inputManager->getMouseY());
            bool leftDown = inputManager->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
            bool leftClicked = inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);

            uiManager->handleInput(mx, my, leftDown, leftClicked);
            uiManager->update(deltaTime);
        }

        // ====================================================================
        // 8. PREPARE AND SUBMIT FRAME DATA
        // ====================================================================
        libre::FrameData frameData = prepareFrameData();
        renderThread->submitFrameData(frameData);

        // ====================================================================
        // 9. UPDATE INPUT STATE
        // ====================================================================
        inputManager->update();
    }

    std::cout << "[MainLoop] Main loop ended" << std::endl;
}

// ============================================================================
// INPUT PROCESSING
// ============================================================================

void Application::processInput(float dt) {
    // ESC to close
    if (inputManager->isKeyPressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window->getHandle(), true);
    }

    // Camera controls
    bool middleMouse = inputManager->isMouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE);
    bool shiftHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);

    if (middleMouse) {
        double dx = inputManager->getMouseX() - lastMouseX;
        double dy = inputManager->getMouseY() - lastMouseY;

        if (shiftHeld) {
            camera->pan(static_cast<float>(-dx) * 0.01f, static_cast<float>(dy) * 0.01f);
        }
        else {
            camera->orbit(static_cast<float>(dx) * 0.5f, static_cast<float>(dy) * 0.5f);
        }
    }

    lastMouseX = inputManager->getMouseX();
    lastMouseY = inputManager->getMouseY();

    // Scroll zoom
    double scrollY = inputManager->getScrollY();
    if (scrollY != 0.0) {
        camera->zoom(static_cast<float>(scrollY) * 0.5f);
    }

    // Selection
    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (!uiManager || !uiManager->isMouseOverUI()) {
            handleSelection();
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
// PREPARE FRAME DATA - WITH DIAGNOSTIC LOGGING
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
    data.light.direction = glm::vec4(glm::normalize(glm::vec3(0.5f, 0.7f, 0.5f)), 0.0f);
    data.light.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    data.light.intensity = 1.0f;
    data.light.ambientStrength = 0.15f;

    // Viewport
    data.viewport.width = static_cast<uint32_t>(std::max(1, w));
    data.viewport.height = static_cast<uint32_t>(std::max(1, h));
    data.viewport.showGrid = true;

    // UI
    data.ui.screenWidth = static_cast<float>(w);
    data.ui.screenHeight = static_cast<float>(h);

    // ========================================================================
    // COLLECT MESHES FROM ECS - WITH DIAGNOSTIC LOGGING
    // ========================================================================
    auto& editor = libre::Editor::instance();
    auto& world = editor.getWorld();

    // Diagnostic counters
    size_t totalMeshComponents = 0;
    size_t meshesNeedingUpload = 0;

    world.forEach<libre::MeshComponent>([&](libre::EntityID id, libre::MeshComponent& meshComp) {
        totalMeshComponents++;

        auto* transform = world.getComponent<libre::TransformComponent>(id);
        auto* render = world.getComponent<libre::RenderComponent>(id);

        if (!transform) return;
        if (render && !render->visible) return;

        // === DIAGNOSTIC: Print mesh component state (first 5 frames only) ===
        if (data.frameNumber <= 5) {
            std::cout << "[prepareFrameData] Entity " << id
                << " | gpuDirty=" << (meshComp.gpuDirty ? "TRUE" : "false")
                << " | vertices=" << meshComp.vertices.size()
                << " | indices=" << meshComp.indices.size() << std::endl;
        }

        // Add to render list
        libre::RenderableMesh rm;
        rm.meshHandle = static_cast<libre::MeshHandle>(id);
        rm.modelMatrix = transform->worldMatrix;
        rm.entityId = id;
        rm.isSelected = editor.isSelected(id);
        rm.color = rm.isSelected ?
            glm::vec4(1.0f, 0.5f, 0.2f, 1.0f) :
            glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

        if (render) {
            rm.color = glm::vec4(render->baseColor, 1.0f);
        }

        data.meshes.push_back(rm);

        // Check if mesh needs GPU upload
        if (meshComp.gpuDirty && !meshComp.vertices.empty()) {
            meshesNeedingUpload++;

            libre::MeshUploadData upload;
            upload.entityId = id;

            // Convert MeshVertex to UploadVertex (drop UV for now)
            upload.vertices.reserve(meshComp.vertices.size());
            for (const auto& mv : meshComp.vertices) {
                libre::UploadVertex v;
                v.position = mv.position;
                v.normal = mv.normal;
                v.color = mv.color;
                upload.vertices.push_back(v);
            }
            upload.indices = meshComp.indices;

            data.meshUploads.push_back(std::move(upload));

            // === DIAGNOSTIC: Log upload ===
            if (data.frameNumber <= 5) {
                std::cout << "[prepareFrameData] >>> QUEUED upload for entity " << id
                    << " (" << meshComp.vertices.size() << " verts, "
                    << meshComp.indices.size() << " indices)" << std::endl;
            }

            // Mark as clean - will be uploaded this frame
            meshComp.gpuDirty = false;
        }
        });

    // === DIAGNOSTIC: Summary (first 5 frames only) ===
    if (data.frameNumber <= 5) {
        std::cout << "[prepareFrameData] Frame " << data.frameNumber
            << " | MeshComponents: " << totalMeshComponents
            << " | NeedUpload: " << meshesNeedingUpload
            << " | Renderables: " << data.meshes.size() << std::endl;
    }

    return data;
}

// ============================================================================
// HELPERS
// ============================================================================

bool Application::isMinimized() const {
    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);
    return (w == 0 || h == 0);
}

void Application::handleSelection() {
    // Placeholder for future selection implementation
}