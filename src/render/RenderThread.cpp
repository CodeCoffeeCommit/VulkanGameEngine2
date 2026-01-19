// src/render/RenderThread.cpp

#include "RenderThread.h"
#include "VulkanContext.h"
#include "SwapChain.h"
#include "Renderer.h"
#include "Mesh.h"
#include "../core/Camera.h"
#include "../core/Window.h"
#include "../core/Editor.h"
#include "../components/CoreComponents.h"

#include <iostream>
#include <chrono>

namespace libre {

    // ============================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ============================================================================

    RenderThread::RenderThread() {
        lastFPSUpdate_ = std::chrono::steady_clock::now();
    }

    RenderThread::~RenderThread() {
        if (running_.load()) {
            stop();
        }
    }

    // ============================================================================
    // LIFECYCLE
    // ============================================================================

    bool RenderThread::start(Window* window) {
        if (running_.load()) {
            std::cerr << "[RenderThread] Already running!" << std::endl;
            return false;
        }

        if (!window) {
            std::cerr << "[RenderThread] Invalid window handle!" << std::endl;
            return false;
        }

        window_ = window;
        shouldStop_.store(false);
        hasError_.store(false);

        thread_ = std::thread(&RenderThread::threadMain, this);

        auto startTime = std::chrono::steady_clock::now();
        while (!running_.load() && !hasError_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            if (std::chrono::steady_clock::now() - startTime > std::chrono::seconds(10)) {
                std::cerr << "[RenderThread] Initialization timeout!" << std::endl;
                shouldStop_.store(true);
                if (thread_.joinable()) {
                    thread_.join();
                }
                return false;
            }
        }

        if (hasError_.load()) {
            std::cerr << "[RenderThread] Initialization failed: " << getErrorMessage() << std::endl;
            if (thread_.joinable()) {
                thread_.join();
            }
            return false;
        }

        std::cout << "[RenderThread] Started successfully" << std::endl;
        return true;
    }

    void RenderThread::stop() {
        if (!running_.load() && !thread_.joinable()) {
            return;
        }

        std::cout << "[RenderThread] Stopping..." << std::endl;

        shouldStop_.store(true, std::memory_order_release);

        if (thread_.joinable()) {
            auto startTime = std::chrono::steady_clock::now();

            while (running_.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                if (std::chrono::steady_clock::now() - startTime > std::chrono::seconds(5)) {
                    std::cerr << "[RenderThread] Warning: Thread didn't stop gracefully" << std::endl;
                    break;
                }
            }

            thread_.join();
        }

        std::cout << "[RenderThread] Stopped" << std::endl;
    }

    std::string RenderThread::getErrorMessage() const {
        std::lock_guard<std::mutex> lock(errorMutex_);
        return errorMessage_;
    }

    // ============================================================================
    // FRAME SUBMISSION
    // ============================================================================

    void RenderThread::submitFrameData(const FrameData& data) {
        int writeIdx = writeBufferIndex_.load(std::memory_order_acquire);
        frameBuffers_[writeIdx] = data;
        newFrameAvailable_.store(true, std::memory_order_release);
    }

    void RenderThread::setUIRenderCallback(std::function<void(void* commandBuffer)> callback) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        uiRenderCallback_ = callback;
    }

    // ============================================================================
    // SWAPCHAIN MANAGEMENT
    // ============================================================================

    void RenderThread::requestSwapchainRecreate(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) {
            return;
        }

        newSwapchainWidth_.store(width, std::memory_order_relaxed);
        newSwapchainHeight_.store(height, std::memory_order_relaxed);
        swapchainRecreateRequested_.store(true, std::memory_order_release);
    }

    void RenderThread::getSwapchainExtent(uint32_t& width, uint32_t& height) const {
        width = currentSwapchainWidth_.load(std::memory_order_acquire);
        height = currentSwapchainHeight_.load(std::memory_order_acquire);
    }

    // ============================================================================
    // RESOURCE MANAGEMENT
    // ============================================================================

    MeshHandle RenderThread::registerMesh(const void* vertices, size_t vertexCount,
        const uint32_t* indices, size_t indexCount,
        uint64_t entityId) {
        return static_cast<MeshHandle>(entityId);
    }

    void RenderThread::unregisterMesh(MeshHandle handle) {
    }

    void RenderThread::updateMeshRegion(MeshHandle handle, uint32_t startVertex,
        uint32_t vertexCount, const void* vertexData) {
    }

    // ============================================================================
    // RENDER THREAD MAIN LOOP
    // ============================================================================

    void RenderThread::threadMain() {
        std::cout << "[RenderThread] Thread started" << std::endl;

        if (!initializeVulkan()) {
            std::lock_guard<std::mutex> lock(errorMutex_);
            errorMessage_ = "Failed to initialize Vulkan";
            hasError_.store(true, std::memory_order_release);
            return;
        }

        running_.store(true, std::memory_order_release);

        // Main render loop
        while (!shouldStop_.load(std::memory_order_acquire)) {
            // Check for swapchain recreation request
            if (swapchainRecreateRequested_.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(swapchainMutex_);
                recreateSwapchain();
                swapchainRecreateRequested_.store(false, std::memory_order_release);
            }

            // Check if window is minimized
            int width = window_->getWidth();
            int height = window_->getHeight();
            if (width == 0 || height == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // Get frame data
            FrameData currentFrameData;
            if (newFrameAvailable_.load(std::memory_order_acquire)) {
                int readIdx = writeBufferIndex_.load(std::memory_order_acquire);
                readBufferIndex_.store(readIdx, std::memory_order_relaxed);
                writeBufferIndex_.store(1 - readIdx, std::memory_order_release);
                newFrameAvailable_.store(false, std::memory_order_release);
                currentFrameData = frameBuffers_[readIdx];
            }
            else {
                int readIdx = readBufferIndex_.load(std::memory_order_acquire);
                currentFrameData = frameBuffers_[readIdx];
            }

            // Render the frame
            renderFrame(currentFrameData);

            // Update FPS counter
            frameCount_++;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<float>(now - lastFPSUpdate_).count();
            if (elapsed >= 1.0f) {
                currentFPS_.store(frameCount_ / elapsed, std::memory_order_relaxed);
                frameCount_ = 0;
                lastFPSUpdate_ = now;
            }
        }

        cleanupVulkan();

        running_.store(false, std::memory_order_release);
        std::cout << "[RenderThread] Thread finished" << std::endl;
    }

    // ============================================================================
    // VULKAN INITIALIZATION
    // ============================================================================

    bool RenderThread::initializeVulkan() {
        try {
            std::cout << "[RenderThread] Initializing Vulkan..." << std::endl;

            vulkanContext_ = std::make_unique<VulkanContext>(window_);
            vulkanContext_->init();

            swapChain_ = std::make_unique<SwapChain>();
            swapChain_->init(vulkanContext_.get(), window_->getHandle());

            VkExtent2D extent = swapChain_->getExtent();
            currentSwapchainWidth_.store(extent.width, std::memory_order_release);
            currentSwapchainHeight_.store(extent.height, std::memory_order_release);

            renderer_ = std::make_unique<Renderer>();
            renderer_->init(vulkanContext_.get(), swapChain_.get());

            std::cout << "[RenderThread] Vulkan initialized successfully" << std::endl;
            return true;

        }
        catch (const std::exception& e) {
            std::cerr << "[RenderThread] Vulkan initialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    void RenderThread::cleanupVulkan() {
        std::cout << "[RenderThread] Cleaning up Vulkan..." << std::endl;

        if (vulkanContext_ && vulkanContext_->getDevice()) {
            vkDeviceWaitIdle(vulkanContext_->getDevice());
        }

        if (renderer_) {
            renderer_->cleanup();
            renderer_.reset();
        }

        if (swapChain_) {
            swapChain_->cleanup();
            swapChain_.reset();
        }

        if (vulkanContext_) {
            vulkanContext_->cleanup();
            vulkanContext_.reset();
        }

        std::cout << "[RenderThread] Vulkan cleanup complete" << std::endl;
    }

    // ============================================================================
    // SYNC ECS TO RENDERER (Called on Render Thread)
    // ============================================================================

    void RenderThread::syncECSToRenderer() {
        if (!renderer_) return;

        // Access ECS from render thread
        auto& world = Editor::instance().getWorld();

        world.forEach<MeshComponent>([&](EntityID id, MeshComponent& meshComp) {
            auto* transform = world.getComponent<TransformComponent>(id);
            auto* render = world.getComponent<RenderComponent>(id);

            if (!transform) return;
            if (render && !render->visible) return;

            // Convert MeshVertex to Vertex for the renderer
            std::vector<Vertex> vulkanVertices;
            vulkanVertices.reserve(meshComp.vertices.size());

            glm::vec3 baseColor = render ? render->baseColor : glm::vec3(0.8f);

            for (const auto& v : meshComp.vertices) {
                Vertex vk;
                vk.position = v.position;
                vk.normal = v.normal;
                vk.color = baseColor;
                vulkanVertices.push_back(vk);
            }

            // Get or create the GPU mesh
            Mesh* mesh = renderer_->getOrCreateMesh(
                id,
                vulkanVertices.data(),
                vulkanVertices.size(),
                meshComp.indices.data(),
                meshComp.indices.size()
            );

            if (mesh) {
                // Check if selected
                bool isSelected = Editor::instance().isSelected(id);
                glm::vec3 color = isSelected ? glm::vec3(1.0f, 0.5f, 0.0f) : baseColor;

                renderer_->submitMesh(mesh, transform->worldMatrix, color, isSelected);
            }
            });
    }

    // ============================================================================
    // RENDER ONE FRAME
    // ============================================================================

    void RenderThread::renderFrame(const FrameData& frameData) {
        if (!renderer_ || !swapChain_ || !vulkanContext_) {
            return;
        }

        // Create camera and set matrices from FrameData
        Camera renderCamera;
        renderCamera.setViewMatrix(frameData.camera.viewMatrix);
        renderCamera.setProjectionMatrix(frameData.camera.projectionMatrix);
        renderCamera.setPosition(frameData.camera.position);
        renderCamera.setAspectRatio(frameData.camera.aspectRatio);

        // Sync meshes from ECS to renderer
        syncECSToRenderer();

        // Draw frame
        bool success = renderer_->drawFrame(&renderCamera);

        if (!success) {
            uint32_t w = currentSwapchainWidth_.load();
            uint32_t h = currentSwapchainHeight_.load();
            if (w > 0 && h > 0) {
                swapchainRecreateRequested_.store(true, std::memory_order_release);
            }
        }

        lastCompletedFrame_.store(frameData.frameNumber, std::memory_order_release);
    }

    // ============================================================================
    // SWAPCHAIN RECREATION
    // ============================================================================

    void RenderThread::recreateSwapchain() {
        uint32_t width = newSwapchainWidth_.load(std::memory_order_acquire);
        uint32_t height = newSwapchainHeight_.load(std::memory_order_acquire);

        if (width == 0 || height == 0) {
            return;
        }

        std::cout << "[RenderThread] Recreating swapchain: " << width << "x" << height << std::endl;

        vkDeviceWaitIdle(vulkanContext_->getDevice());

        swapChain_->recreate(window_->getHandle());

        VkExtent2D extent = swapChain_->getExtent();
        currentSwapchainWidth_.store(extent.width, std::memory_order_release);
        currentSwapchainHeight_.store(extent.height, std::memory_order_release);

        renderer_->onSwapChainRecreated(swapChain_.get());

        std::cout << "[RenderThread] Swapchain recreated successfully" << std::endl;
    }

} // namespace libre