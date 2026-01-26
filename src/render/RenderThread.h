// src/render/RenderThread.h
// COMPLETE FILE - Using unique_ptr for FrameData buffers

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <memory>
#include <chrono>
#include <string>

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

        // Lifecycle
        bool start(Window* window);
        void stop();
        bool isRunning() const { return running_.load(std::memory_order_acquire); }
        bool hasError() const { return hasError_.load(std::memory_order_acquire); }
        std::string getErrorMessage() const;

        // Frame submission
        void submitFrameData(const FrameData& data);
        void setUIRenderCallback(std::function<void(void* commandBuffer)> callback);

        // Swapchain management
        void requestSwapchainRecreate(uint32_t width, uint32_t height);
        void getSwapchainExtent(uint32_t& width, uint32_t& height) const;

        // Vulkan access (for UI initialization)
        VulkanContext* getVulkanContext() const { return vulkanContext_.get(); }
        VkRenderPass getRenderPass() const;

        // Resource management
        MeshHandle registerMesh(const void* vertices, size_t vertexCount,
            const uint32_t* indices, size_t indexCount,
            uint64_t entityId = 0);
        void unregisterMesh(MeshHandle handle);
        void updateMeshRegion(MeshHandle handle, uint32_t startVertex,
            uint32_t vertexCount, const void* vertexData);

        // Statistics
        uint64_t getLastCompletedFrame() const {
            return lastCompletedFrame_.load(std::memory_order_acquire);
        }
        float getCurrentFPS() const { return currentFPS_.load(std::memory_order_relaxed); }

        Window* getWindow() const { return window_; }

    private:
        // Private methods
        void threadMain();
        bool initializeVulkan();
        void cleanupVulkan();
        void handleSwapchainRecreate();
        void renderFrame(const FrameData& frameData);
        void syncECSToRenderer();

        // Thread state
        std::thread thread_;
        std::atomic<bool> running_{ false };
        std::atomic<bool> shouldStop_{ false };
        std::atomic<bool> hasError_{ false };
        std::string errorMessage_;
        mutable std::mutex errorMutex_;

        // Frame data double buffer - using unique_ptr to avoid MSVC template ICE
        std::unique_ptr<FrameData> frameBuffer0_;
        std::unique_ptr<FrameData> frameBuffer1_;
        std::atomic<int> writeBufferIndex_{ 0 };
        std::atomic<int> readBufferIndex_{ 1 };
        std::atomic<bool> newFrameAvailable_{ false };

        // Swapchain recreation
        std::atomic<bool> swapchainRecreateRequested_{ false };
        std::atomic<uint32_t> newSwapchainWidth_{ 0 };
        std::atomic<uint32_t> newSwapchainHeight_{ 0 };
        std::mutex swapchainMutex_;
        std::atomic<uint32_t> currentSwapchainWidth_{ 0 };
        std::atomic<uint32_t> currentSwapchainHeight_{ 0 };

        // Vulkan objects (owned by render thread)
        std::unique_ptr<VulkanContext> vulkanContext_;
        std::unique_ptr<SwapChain> swapChain_;
        std::unique_ptr<Renderer> renderer_;
        Window* window_ = nullptr;

        // Callbacks
        std::function<void(void*)> uiRenderCallback_;
        mutable std::mutex callbackMutex_;

        // Statistics
        std::atomic<uint64_t> lastCompletedFrame_{ 0 };
        std::atomic<float> currentFPS_{ 0.0f };
        uint64_t frameCount_ = 0;
        std::chrono::steady_clock::time_point lastFPSUpdate_;

        // Helper to get frame buffer by index
        FrameData& getFrameBuffer(int index) {
            return (index == 0) ? *frameBuffer0_ : *frameBuffer1_;
        }
        const FrameData& getFrameBuffer(int index) const {
            return (index == 0) ? *frameBuffer0_ : *frameBuffer1_;
        }
    };

} // namespace libre