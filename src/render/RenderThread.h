// src/render/RenderThread.h

#pragma once

// CRITICAL: Include Vulkan BEFORE other headers for VkRenderPass type
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <memory>
#include <chrono>

#include "../core/FrameData.h"

// Forward declarations
class VulkanContext;
class SwapChain;
class Renderer;
class Camera;
class Window;

namespace libre {

    class RenderThread {
    public:
        RenderThread();
        ~RenderThread();

        // Non-copyable, non-movable
        RenderThread(const RenderThread&) = delete;
        RenderThread& operator=(const RenderThread&) = delete;
        RenderThread(RenderThread&&) = delete;
        RenderThread& operator=(RenderThread&&) = delete;

        // ========================================================================
        // LIFECYCLE
        // ========================================================================

        bool start(Window* window);
        void stop();
        bool isRunning() const { return running_.load(std::memory_order_acquire); }
        bool hasError() const { return hasError_.load(std::memory_order_acquire); }
        std::string getErrorMessage() const;

        // ========================================================================
        // FRAME SUBMISSION
        // ========================================================================

        void submitFrameData(const FrameData& data);
        void setUIRenderCallback(std::function<void(void* commandBuffer)> callback);

        // ========================================================================
        // SWAPCHAIN MANAGEMENT
        // ========================================================================

        void requestSwapchainRecreate(uint32_t width, uint32_t height);
        bool isSwapchainRecreatePending() const {
            return swapchainRecreateRequested_.load(std::memory_order_acquire);
        }

        // ========================================================================
        // VULKAN ACCESS (for UI initialization)
        // These provide read-only access to Vulkan objects for UI setup
        // ========================================================================

        VulkanContext* getVulkanContext() const { return vulkanContext_.get(); }
        SwapChain* getSwapChain() const { return swapChain_.get(); }
        Renderer* getRenderer() const { return renderer_.get(); }

        // Get render pass for UI pipeline creation
        VkRenderPass getRenderPass() const;

        // ========================================================================
        // RESOURCE MANAGEMENT
        // ========================================================================

        MeshHandle registerMesh(const void* vertices, size_t vertexCount,
            const uint32_t* indices, size_t indexCount,
            uint64_t entityId);
        void unregisterMesh(MeshHandle handle);
        void updateMeshRegion(MeshHandle handle, uint32_t startVertex,
            uint32_t vertexCount, const void* vertexData);

        // ========================================================================
        // QUERIES
        // ========================================================================

        void getSwapchainExtent(uint32_t& width, uint32_t& height) const;
        uint64_t getLastCompletedFrame() const {
            return lastCompletedFrame_.load(std::memory_order_acquire);
        }
        float getCurrentFPS() const { return currentFPS_.load(std::memory_order_relaxed); }

        // Get window handle (needed for swapchain recreate)
        Window* getWindow() const { return window_; }

    private:
        // ========================================================================
        // RENDER THREAD INTERNALS
        // ========================================================================

        void threadMain();
        bool initializeVulkan();
        void cleanupVulkan();
        void renderFrame(const FrameData& frameData);
        void recreateSwapchain();
        void syncECSToRenderer();

        // ========================================================================
        // THREAD STATE
        // ========================================================================

        std::thread thread_;
        std::atomic<bool> running_{ false };
        std::atomic<bool> shouldStop_{ false };
        std::atomic<bool> hasError_{ false };
        std::string errorMessage_;
        mutable std::mutex errorMutex_;

        // ========================================================================
        // FRAME DATA DOUBLE BUFFER
        // ========================================================================

        FrameData frameBuffers_[2];
        std::atomic<int> writeBufferIndex_{ 0 };
        std::atomic<int> readBufferIndex_{ 1 };
        std::atomic<bool> newFrameAvailable_{ false };

        // ========================================================================
        // SWAPCHAIN RECREATION
        // ========================================================================

        std::atomic<bool> swapchainRecreateRequested_{ false };
        std::atomic<uint32_t> newSwapchainWidth_{ 0 };
        std::atomic<uint32_t> newSwapchainHeight_{ 0 };
        std::mutex swapchainMutex_;

        std::atomic<uint32_t> currentSwapchainWidth_{ 0 };
        std::atomic<uint32_t> currentSwapchainHeight_{ 0 };

        // ========================================================================
        // VULKAN OBJECTS
        // ========================================================================

        std::unique_ptr<VulkanContext> vulkanContext_;
        std::unique_ptr<SwapChain> swapChain_;
        std::unique_ptr<Renderer> renderer_;
        Window* window_ = nullptr;

        // ========================================================================
        // CALLBACKS
        // ========================================================================

        std::function<void(void*)> uiRenderCallback_;
        mutable std::mutex callbackMutex_;

        // ========================================================================
        // STATISTICS
        // ========================================================================

        std::atomic<uint64_t> lastCompletedFrame_{ 0 };
        std::atomic<float> currentFPS_{ 0.0f };

        uint64_t frameCount_ = 0;
        std::chrono::steady_clock::time_point lastFPSUpdate_;
    };

} // namespace libre