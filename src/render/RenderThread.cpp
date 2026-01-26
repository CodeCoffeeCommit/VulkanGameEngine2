// src/render/RenderThread.cpp
// COMPLETE FILE - Replace your existing RenderThread.cpp with this

#include "RenderThread.h"
#include "VulkanContext.h"
#include "SwapChain.h"
#include "Renderer.h"
#include "../core/Window.h"
#include "../core/Camera.h"
#include <iostream>
#include <chrono>

namespace libre {

    RenderThread::RenderThread() {}

    RenderThread::~RenderThread() {
        stop();
    }

    bool RenderThread::start(Window* window) {
        if (running_.load()) {
            std::cerr << "[RenderThread] Already running!" << std::endl;
            return false;
        }

        if (!window) {
            std::cerr << "[RenderThread] Window is null!" << std::endl;
            return false;
        }

        window_ = window;
        shouldStop_.store(false);
        hasError_.store(false);

        thread_ = std::thread(&RenderThread::threadMain, this);

        // Wait for initialization
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
        uiRenderCallback_ = std::move(callback);
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
    // VULKAN ACCESS FOR UI
    // ============================================================================

    VkRenderPass RenderThread::getRenderPass() const {
        if (swapChain_) {
            return swapChain_->getRenderPass();
        }
        return VK_NULL_HANDLE;
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
        // TODO: Implement mesh unregistration
    }

    void RenderThread::updateMeshRegion(MeshHandle handle, uint32_t startVertex,
        uint32_t vertexCount, const void* vertexData) {
        // TODO: Implement partial mesh updates
    }

    // ============================================================================
    // THREAD MAIN LOOP
    // ============================================================================

    void RenderThread::threadMain() {
        std::cout << "[RenderThread] Thread started" << std::endl;

        if (!initializeVulkan()) {
            std::lock_guard<std::mutex> lock(errorMutex_);
            if (errorMessage_.empty()) {
                errorMessage_ = "Vulkan initialization failed";
            }
            hasError_.store(true);
            return;
        }

        running_.store(true, std::memory_order_release);
        std::cout << "[RenderThread] Running" << std::endl;

        while (!shouldStop_.load(std::memory_order_acquire)) {
            // Handle swapchain recreation
            if (swapchainRecreateRequested_.load(std::memory_order_acquire)) {
                handleSwapchainRecreate();
                swapchainRecreateRequested_.store(false, std::memory_order_release);
            }

            // Check for new frame data
            if (newFrameAvailable_.load(std::memory_order_acquire)) {
                int readIdx = writeBufferIndex_.load(std::memory_order_acquire);
                writeBufferIndex_.store(1 - readIdx, std::memory_order_release);
                newFrameAvailable_.store(false, std::memory_order_release);

                renderFrame(frameBuffers_[readIdx]);
            }
            else {
                // No new frame, sleep briefly to avoid spinning
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }

        // Cleanup
        std::cout << "[RenderThread] Shutting down..." << std::endl;

        if (vulkanContext_ && vulkanContext_->getDevice()) {
            vkDeviceWaitIdle(vulkanContext_->getDevice());
        }

        cleanupVulkan();

        running_.store(false, std::memory_order_release);
        std::cout << "[RenderThread] Thread ended" << std::endl;
    }

    // ============================================================================
    // VULKAN INITIALIZATION
    // ============================================================================

    bool RenderThread::initializeVulkan() {
        try {
            std::cout << "[RenderThread] Initializing Vulkan..." << std::endl;

            vulkanContext_ = std::make_unique<VulkanContext>();
            vulkanContext_->init(window_->getHandle());

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
            std::lock_guard<std::mutex> lock(errorMutex_);
            errorMessage_ = e.what();
            std::cerr << "[RenderThread] Vulkan initialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    void RenderThread::cleanupVulkan() {
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
    }

    void RenderThread::handleSwapchainRecreate() {
        uint32_t newWidth = newSwapchainWidth_.load(std::memory_order_acquire);
        uint32_t newHeight = newSwapchainHeight_.load(std::memory_order_acquire);

        std::cout << "[RenderThread] Recreating swapchain: " << newWidth << "x" << newHeight << std::endl;

        vkDeviceWaitIdle(vulkanContext_->getDevice());

        swapChain_->recreate(window_->getHandle());

        VkExtent2D extent = swapChain_->getExtent();
        currentSwapchainWidth_.store(extent.width, std::memory_order_release);
        currentSwapchainHeight_.store(extent.height, std::memory_order_release);

        renderer_->onSwapChainRecreated(swapChain_.get());

        std::cout << "[RenderThread] Swapchain recreated: " << extent.width << "x" << extent.height << std::endl;
    }

    // ============================================================================
    // RENDER FRAME
    // ============================================================================

    void RenderThread::renderFrame(const FrameData& frameData) {
        if (!renderer_ || !swapChain_) return;

        // Clear previous submissions
        renderer_->clearSubmissions();

        // =========================================================================
        // STEP 1: Upload any new/dirty meshes to GPU
        // =========================================================================
        for (const auto& upload : frameData.meshUploads) {
            if (!upload.vertices.empty()) {
                renderer_->getOrCreateMesh(
                    upload.entityId,
                    upload.vertices.data(),
                    upload.vertices.size(),
                    upload.indices.data(),
                    upload.indices.size()
                );
            }
        }

        // =========================================================================
        // STEP 2: Submit meshes for rendering
        // =========================================================================
        for (const auto& rm : frameData.meshes) {
            auto* mesh = renderer_->getMeshFromCache(rm.entityId);
            if (mesh) {
                renderer_->submitMesh(
                    mesh,
                    rm.modelMatrix,
                    glm::vec3(rm.color),
                    rm.isSelected
                );
            }
        }

        // =========================================================================
        // STEP 3: Setup UI callback - FIXED: Avoid deadlock!
        // =========================================================================
        // Copy the callback OUTSIDE the lock scope to avoid deadlock.
        // The previous version held callbackMutex_ while setting up a lambda that
        // also tried to acquire callbackMutex_ when invoked.
        std::function<void(void*)> uiCallback;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            uiCallback = uiRenderCallback_;
        }
        // Mutex is now released BEFORE we set up the renderer callback

        if (uiCallback) {
            renderer_->setUIRenderCallback([uiCallback](VkCommandBuffer cmd) {
                uiCallback(static_cast<void*>(cmd));
                });
        }
        else {
            renderer_->setUIRenderCallback(nullptr);
        }

        // =========================================================================
        // STEP 4: Create camera and draw frame
        // =========================================================================
        Camera tempCamera;
        tempCamera.setViewMatrix(frameData.camera.viewMatrix);
        tempCamera.setProjectionMatrix(frameData.camera.projectionMatrix);
        tempCamera.setPosition(frameData.camera.position);

        bool success = renderer_->drawFrame(&tempCamera);

        if (!success) {
            // Swapchain needs recreation - will be handled next frame
            swapchainRecreateRequested_.store(true, std::memory_order_release);
        }

        lastCompletedFrame_.store(frameData.frameNumber, std::memory_order_release);
    }

    // ============================================================================
    // SYNC ECS TO RENDERER
    // ============================================================================

    void RenderThread::syncECSToRenderer() {
        // This is handled in renderFrame via FrameData
    }

} // namespace libre