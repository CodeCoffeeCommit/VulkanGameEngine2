// src/render/RenderThread.cpp
// COMPLETE FILE - Replace your existing RenderThread.cpp with this
// FIXED VERSION - Corrects double-buffer swap logic and adds diagnostic logging

#include "RenderThread.h"
#include "VulkanContext.h"
#include "SwapChain.h"
#include "Renderer.h"
#include "Mesh.h"
#include "../core/Window.h"
#include "../core/Camera.h"
#include <iostream>
#include <chrono>

namespace libre {

    RenderThread::RenderThread() {
        // Initialize both frame buffers
        frameBuffers_[0] = std::make_unique<FrameData>();
        frameBuffers_[1] = std::make_unique<FrameData>();
    }

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

        // Reset buffer indices
        writeBufferIndex_.store(0, std::memory_order_release);
        readBufferIndex_.store(1, std::memory_order_release);
        newFrameAvailable_.store(false, std::memory_order_release);

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

    void RenderThread::submitFrameData(const FrameData& data) {
        // Get current write buffer index
        int writeIdx = writeBufferIndex_.load(std::memory_order_acquire);

        // Copy data to write buffer
        *frameBuffers_[writeIdx] = data;

        // Swap buffers: render thread will now read from this buffer
        // and we'll write to the other one next time
        int newWriteIdx = 1 - writeIdx;
        writeBufferIndex_.store(newWriteIdx, std::memory_order_release);
        readBufferIndex_.store(writeIdx, std::memory_order_release);

        // Signal new frame available
        newFrameAvailable_.store(true, std::memory_order_release);
    }

    void RenderThread::setUIRenderCallback(std::function<void(void* commandBuffer)> callback) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        uiRenderCallback_ = std::move(callback);
    }

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

    VkRenderPass RenderThread::getRenderPass() const {
        if (swapChain_) {
            return swapChain_->getRenderPass();
        }
        return VK_NULL_HANDLE;
    }

    MeshHandle RenderThread::registerMesh(const void* vertices, size_t vertexCount,
        const uint32_t* indices, size_t indexCount, uint64_t entityId) {
        return static_cast<MeshHandle>(entityId);
    }

    void RenderThread::unregisterMesh(MeshHandle handle) {
        // TODO: Implement
    }

    void RenderThread::updateMeshRegion(MeshHandle handle, uint32_t startVertex,
        uint32_t vertexCount, const void* vertexData) {
        // TODO: Implement
    }

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
            // Check for swapchain recreation request
            if (swapchainRecreateRequested_.load(std::memory_order_acquire)) {
                handleSwapchainRecreate();
                swapchainRecreateRequested_.store(false, std::memory_order_release);
            }

            // Check for new frame data
            if (newFrameAvailable_.load(std::memory_order_acquire)) {
                // Read from the buffer that was just written
                int readIdx = readBufferIndex_.load(std::memory_order_acquire);
                newFrameAvailable_.store(false, std::memory_order_release);

                // Render the frame
                *renderFrame(frameBuffers_[readIdx]);
            }
            else {
                // No new frame, sleep briefly to avoid spinning
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }

        std::cout << "[RenderThread] Shutting down..." << std::endl;

        if (vulkanContext_ && vulkanContext_->getDevice()) {
            vkDeviceWaitIdle(vulkanContext_->getDevice());
        }

        cleanupVulkan();
        running_.store(false, std::memory_order_release);
        std::cout << "[RenderThread] Thread ended" << std::endl;
    }

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
            std::lock_guard<std::mutex> lock(errorMutex_);
            errorMessage_ = e.what();
            std::cerr << "[RenderThread] Vulkan init error: " << e.what() << std::endl;
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

    void RenderThread::renderFrame(const FrameData& frameData) {
        if (!renderer_ || !swapChain_) return;

        renderer_->clearSubmissions();

        // === STEP 1: Process mesh uploads FIRST ===
        for (const auto& upload : frameData.meshUploads) {
            if (!upload.vertices.empty() && !upload.indices.empty()) {
                // Log uploads for first few frames
                if (frameData.frameNumber <= 5) {
                    std::cout << "[RenderThread] Uploading mesh for entity " << upload.entityId
                        << " (" << upload.vertices.size() << " verts, "
                        << upload.indices.size() << " indices)" << std::endl;
                }

                Mesh* mesh = renderer_->getOrCreateMesh(
                    upload.entityId,
                    upload.vertices.data(),
                    upload.vertices.size(),
                    upload.indices.data(),
                    upload.indices.size()
                );

                if (!mesh && frameData.frameNumber <= 5) {
                    std::cerr << "[RenderThread] ERROR: Failed to create mesh " << upload.entityId << std::endl;
                }
            }
        }

        // === STEP 2: Submit meshes for rendering ===
        size_t submitted = 0;
        size_t notFound = 0;

        for (const auto& rm : frameData.meshes) {
            Mesh* mesh = renderer_->getMeshFromCache(rm.entityId);
            if (mesh) {
                renderer_->submitMesh(mesh, rm.modelMatrix, glm::vec3(rm.color), rm.isSelected);
                submitted++;
            }
            else {
                notFound++;
                if (frameData.frameNumber <= 10) {
                    std::cerr << "[RenderThread] WARNING: Mesh " << rm.entityId
                        << " not found in cache (frame " << frameData.frameNumber << ")" << std::endl;
                }
            }
        }

        // Debug summary for first few frames
        if (frameData.frameNumber <= 5) {
            std::cout << "[RenderThread] Frame " << frameData.frameNumber
                << " | Uploads: " << frameData.meshUploads.size()
                << " | ToRender: " << frameData.meshes.size()
                << " | Submitted: " << submitted
                << " | NotFound: " << notFound << std::endl;
        }

        // === STEP 3: Setup UI callback ===
        std::function<void(void*)> uiCallback;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            uiCallback = uiRenderCallback_;
        }

        if (uiCallback) {
            renderer_->setUIRenderCallback([uiCallback](VkCommandBuffer cmd) {
                uiCallback(static_cast<void*>(cmd));
                });
        }
        else {
            renderer_->setUIRenderCallback(nullptr);
        }

        // === STEP 4: Draw frame ===
        Camera tempCamera;
        tempCamera.setViewMatrix(frameData.camera.viewMatrix);
        tempCamera.setProjectionMatrix(frameData.camera.projectionMatrix);
        tempCamera.setPosition(frameData.camera.position);

        bool success = renderer_->drawFrame(&tempCamera);
        if (!success) {
            swapchainRecreateRequested_.store(true, std::memory_order_release);
        }

        lastCompletedFrame_.store(frameData.frameNumber, std::memory_order_release);
    }

    void RenderThread::syncECSToRenderer() {
        // No longer needed - handled via FrameData
    }

} // namespace libre