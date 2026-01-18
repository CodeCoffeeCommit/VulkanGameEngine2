#include "Application.h"
#include "Editor.h"
#include "Selection.h"
#include "../render/SwapChain.h"
#include "../render/Renderer.h"
#include "../render/Mesh.h"
#include "../world/Primitives.h"
#include "../ui/UI.h"
#include "../ui/Widgets.h"
#include "../ui/PreferencesWindow.h"
#include <iostream>
#include "..//ShaderCompiler.h"

Application::Application() {
    std::cout << "====================================" << std::endl;
    std::cout << "LIBRE DCC TOOL - 3D Viewport" << std::endl;
    std::cout << "====================================" << std::endl;
}

Application::~Application() {
    cleanup();
}

void Application::run() {
    init();
    mainLoop();
}

void Application::setupUI() {
    using namespace libre::ui;

    uiManager = std::make_unique<UIManager>();
    uiManager->init(vulkanContext.get(), swapChain->getRenderPass());

    // Create Menu Bar
    auto menuBar = std::make_unique<MenuBar>();

    // File Menu
    menuBar->addMenu("File", {
        MenuItem("New Project", []() { std::cout << "[Menu] New Project" << std::endl; }),
        MenuItem("Open...", []() { std::cout << "[Menu] Open" << std::endl; }),
        MenuItem("Save", []() { std::cout << "[Menu] Save" << std::endl; }),
        MenuItem::Separator(),
        MenuItem("Exit", [this]() { glfwSetWindowShouldClose(window->getHandle(), GLFW_TRUE); })
        });

    // Edit Menu
    menuBar->addMenu("Edit", {
        MenuItem("Undo", []() { std::cout << "[Menu] Undo" << std::endl; }),
        MenuItem("Redo", []() { std::cout << "[Menu] Redo" << std::endl; }),
        MenuItem::Separator(),
        MenuItem("Preferences...", [this]() { openPreferencesWindow(); })
        });

    // Help Menu
    menuBar->addMenu("Help", {
        MenuItem("Documentation", []() { std::cout << "[Menu] Documentation" << std::endl; }),
        MenuItem::Separator(),
        MenuItem("About", [this]() { openPreferencesWindow(); })
        });

    uiManager->setMenuBar(std::move(menuBar));

    // Create Preferences Window
    auto prefsWin = std::make_unique<PreferencesWindow>();
    preferencesWindow = prefsWin.get();
    uiManager->addWindow(std::move(prefsWin));

    // Initial layout
    int w, h;
    glfwGetFramebufferSize(window->getHandle(), &w, &h);
    uiManager->layout(static_cast<float>(w), static_cast<float>(h));


    // Set UI render callback
    renderer->setUIRenderCallback([this](VkCommandBuffer cmd) {
        uiManager->render(cmd);
        });


    std::cout << "[OK] UI System initialized" << std::endl;
}

void Application::openPreferencesWindow() {
    if (preferencesWindow) {
        preferencesWindow->isOpen = true;
        int w, h;
        glfwGetFramebufferSize(window->getHandle(), &w, &h);
        preferencesWindow->bounds.x = (w - preferencesWindow->bounds.w) / 2.0f;
        preferencesWindow->bounds.y = (h - preferencesWindow->bounds.h) / 2.0f;
        preferencesWindow->layout(preferencesWindow->bounds);
    }
}
void Application::init() {
    std::cout << "\n[INITIALIZATION]" << std::endl;

    if (!ShaderCompiler::compileAllShaders()) {
        throw std::runtime_error("Failed to compile shaders!");
    }

    window = std::make_unique<Window>(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    inputManager = std::make_unique<InputManager>(window.get());

    libre::Editor::instance().initialize();

    camera = std::make_unique<Camera>();
    camera->setAspectRatio(static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT));

    vulkanContext = std::make_unique<VulkanContext>(window.get());
    vulkanContext->init();

    swapChain = std::make_unique<SwapChain>();
    swapChain->init(vulkanContext.get(), window->getHandle());

    renderer = std::make_unique<Renderer>();
    renderer->init(vulkanContext.get(), swapChain.get());

    setupUI();
    createDefaultScene();

    lastFrameTime = std::chrono::steady_clock::now();

    std::cout << "\n[OK] Application initialized successfully!" << std::endl;
    printControls();
}

void Application::createDefaultScene() {
    auto& world = libre::Editor::instance().getWorld();

    auto cube = libre::Primitives::createCube(world, 2.0f, "DefaultCube");

    auto sphere = libre::Primitives::createSphere(world, 1.0f, 32, 16, "Sphere");
    if (auto* t = sphere.get<libre::TransformComponent>()) {
        t->position = glm::vec3(3.0f, 0.0f, 0.0f);
        t->dirty = true;
    }

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
    std::cout << "Middle Mouse + Drag: Orbit" << std::endl;
    std::cout << "Shift + Middle Mouse: Pan" << std::endl;
    std::cout << "Scroll Wheel: Zoom" << std::endl;
    std::cout << "Left Click: Select" << std::endl;
    std::cout << "Shift + Left Click: Add to Selection" << std::endl;
    std::cout << "A: Select All" << std::endl;
    std::cout << "Alt+A: Deselect All" << std::endl;
    std::cout << "Delete/X: Delete Selected" << std::endl;
    std::cout << "Ctrl+Z: Undo" << std::endl;
    std::cout << "Ctrl+Shift+Z: Redo" << std::endl;
    std::cout << "Numpad 1/3/7/0: View shortcuts" << std::endl;
    std::cout << "F11: Toggle Fullscreen" << std::endl;
    std::cout << "ESC: Exit" << std::endl;
    std::cout << "================\n" << std::endl;
}

bool Application::isMinimized() const {
    int width, height;
    glfwGetFramebufferSize(window->getHandle(), &width, &height);
    return width == 0 || height == 0;
}

void Application::handleResize() {
    // Wait while minimized
    while (isMinimized()) {
        glfwWaitEvents();
    }

    // Wait for GPU to finish
    renderer->waitIdle();

    // Get new dimensions
    int width, height;
    glfwGetFramebufferSize(window->getHandle(), &width, &height);

    std::cout << "[Resize] New size: " << width << "x" << height << std::endl;

    // Update camera aspect ratio
    camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));

    // Recreate swap chain (this also recreates framebuffers)
    swapChain->recreate(window->getHandle());

    // Renderer needs to know about new swap chain
    renderer->onSwapChainRecreated(swapChain.get());

    framebufferResized = false;
}

void Application::mainLoop() {
    while (!window->shouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        fps = 1.0f / deltaTime;

        // Poll events first
        window->pollEvents();

        // FIXED: Check for resize BEFORE rendering
        if (window->wasResized() || framebufferResized) {
            window->resetResizeFlag();
            handleResize();
            continue;  // Skip this frame, start fresh
        }

        // Skip rendering if minimized
        if (isMinimized()) {
            continue;
        }

        // Process input
        processInput(deltaTime);

        // Update game state
        update(deltaTime);

        // Render - may set framebufferResized if swap chain is out of date
        render();

        // Update input state for next frame
        inputManager->update();
    }

    // Wait for GPU before cleanup
    renderer->waitIdle();
}

void Application::processInput(float dt) {
    auto& editor = libre::Editor::instance();

    if (inputManager->isKeyPressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window->getHandle(), GLFW_TRUE);
    }

    // Toggle fullscreen with F11
    if (inputManager->isKeyJustPressed(GLFW_KEY_F11)) {
        static bool isFullscreen = false;
        static int savedX, savedY, savedWidth, savedHeight;

        if (!isFullscreen) {
            // Save current window position/size
            glfwGetWindowPos(window->getHandle(), &savedX, &savedY);
            glfwGetWindowSize(window->getHandle(), &savedWidth, &savedHeight);

            // Get primary monitor
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            // Go fullscreen
            glfwSetWindowMonitor(window->getHandle(), monitor, 0, 0,
                mode->width, mode->height, mode->refreshRate);
            isFullscreen = true;
        }
        else {
            // Restore windowed mode
            glfwSetWindowMonitor(window->getHandle(), nullptr,
                savedX, savedY, savedWidth, savedHeight, 0);
            isFullscreen = false;
        }

        // Mark for resize handling
        framebufferResized = true;
    }

    shiftHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
    ctrlHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_CONTROL);
    altHeld = inputManager->isKeyPressed(GLFW_KEY_LEFT_ALT) ||
        inputManager->isKeyPressed(GLFW_KEY_RIGHT_ALT);

    if (ctrlHeld && inputManager->isKeyJustPressed(GLFW_KEY_Z)) {
        if (shiftHeld) {
            editor.redo();
            std::cout << "[Redo]" << std::endl;
        }
        else {
            editor.undo();
            std::cout << "[Undo]" << std::endl;
        }
    }

    if (inputManager->isKeyJustPressed(GLFW_KEY_DELETE) ||
        inputManager->isKeyJustPressed(GLFW_KEY_X)) {
        editor.deleteSelected();
    }

    if (inputManager->isKeyJustPressed(GLFW_KEY_A)) {
        if (altHeld) {
            editor.deselectAll();
        }
        else {
            editor.selectAll();
        }
    }

    if (inputManager->isKeyJustPressed(GLFW_KEY_KP_1)) camera->setFront();
    if (inputManager->isKeyJustPressed(GLFW_KEY_KP_3)) camera->setRight();
    if (inputManager->isKeyJustPressed(GLFW_KEY_KP_7)) camera->setTop();
    if (inputManager->isKeyJustPressed(GLFW_KEY_KP_0)) camera->reset();

    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (!middleMouseDown) {
            handleSelection();
        }
    }

    if (inputManager->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_MIDDLE)) {
        middleMouseDown = true;
        lastMouseX = inputManager->getMouseX();
        lastMouseY = inputManager->getMouseY();
    }
    if (inputManager->isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_MIDDLE)) {
        middleMouseDown = false;
    }

    if (middleMouseDown) {
        double currentX = inputManager->getMouseX();
        double currentY = inputManager->getMouseY();
        float deltaX = static_cast<float>(currentX - lastMouseX);
        float deltaY = static_cast<float>(currentY - lastMouseY);

        if (shiftHeld) {
            camera->pan(deltaX, deltaY);
        }
        else {
            camera->orbit(deltaX, deltaY);
        }

        lastMouseX = currentX;
        lastMouseY = currentY;
    }

    double scrollY = inputManager->getScrollY();
    if (scrollY != 0.0) {
        camera->zoom(static_cast<float>(scrollY));
    }
}

void Application::handleSelection() {
    auto& editor = libre::Editor::instance();
    auto& world = editor.getWorld();

    int width, height;
    glfwGetFramebufferSize(window->getHandle(), &width, &height);

    float mouseX = static_cast<float>(inputManager->getMouseX());
    float mouseY = static_cast<float>(inputManager->getMouseY());

    libre::Ray ray = libre::SelectionSystem::screenToRay(
        *camera, mouseX, mouseY, width, height);

    libre::HitResult hit = libre::SelectionSystem::raycast(world, ray);

    if (hit.hit()) {
        editor.select(hit.entity, shiftHeld);

        auto* meta = world.getMetadata(hit.entity);
        if (meta) {
            std::cout << "[Selected] " << meta->name
                << " (distance: " << hit.distance << ")" << std::endl;
        }
    }
    else if (!shiftHeld) {
        editor.deselectAll();
    }
}

void Application::update(float dt) {
    libre::Editor::instance().update(dt);
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

void Application::render() {
    syncECSToRenderer();

    // drawFrame returns false if swap chain needs recreation
    if (!renderer->drawFrame(camera.get())) {
        framebufferResized = true;
    }
}

void Application::syncECSToRenderer() {
    auto& world = libre::Editor::instance().getWorld();

    static bool debugPrinted = false;
    int entityCount = 0;

    world.forEach<libre::MeshComponent>([&](libre::EntityID id, libre::MeshComponent& meshComp) {
        auto* transform = world.getComponent<libre::TransformComponent>(id);
        auto* render = world.getComponent<libre::RenderComponent>(id);

        if (!debugPrinted) {
            auto* meta = world.getMetadata(id);
            std::cout << "[Sync] Entity: " << (meta ? meta->name : "?")
                << " ID=" << id
                << " Verts=" << meshComp.vertices.size()
                << " Indices=" << meshComp.indices.size() << std::endl;
        }

        if (!transform || !render || !render->visible) {
            return;
        }

        // Convert MeshVertex to Vertex
        std::vector<Vertex> vulkanVertices;
        vulkanVertices.reserve(meshComp.vertices.size());

        for (const auto& v : meshComp.vertices) {
            Vertex vk;
            vk.position = v.position;
            vk.normal = v.normal;
            vk.color = render->baseColor;
            vulkanVertices.push_back(vk);
        }

        Mesh* gpuMesh = renderer->getOrCreateMesh(
            id,
            vulkanVertices.data(),
            vulkanVertices.size(),
            meshComp.indices.data(),
            meshComp.indices.size()
        );

        bool selected = world.isSelected(id);
        glm::vec3 color = selected ? glm::vec3(1.0f, 0.6f, 0.2f) : render->baseColor;

        renderer->submitMesh(gpuMesh, transform->worldMatrix, color, selected);
        entityCount++;
        });

    if (!debugPrinted) {
        std::cout << "[Sync] Total: " << entityCount << " entities" << std::endl;
        debugPrinted = true;
    }
}



void Application::cleanup() {
    std::cout << "\n[CLEANUP]" << std::endl;

    if (renderer) {
        renderer->waitIdle();

        renderer->cleanup();
        renderer.reset();
    }

    if (swapChain) {
        swapChain->cleanup();
        swapChain.reset();
    }

    libre::Editor::instance().shutdown();

    if (vulkanContext) {
        vulkanContext->cleanup();
        vulkanContext.reset();
    }

    camera.reset();
    inputManager.reset();
    window.reset();

    std::cout << "[OK] Application cleaned up" << std::endl;
}