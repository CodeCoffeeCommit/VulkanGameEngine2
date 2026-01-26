// src/render/Renderer.h

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>

// Forward declarations
class VulkanContext;
class SwapChain;
class GraphicsPipeline;
class UniformBuffer;
class Grid;
class Mesh;
class Camera;

struct RenderObject {
    Mesh* mesh = nullptr;
    glm::mat4 transform = glm::mat4(1.0f);
    glm::vec3 color = glm::vec3(0.8f);
    bool selected = false;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    using UIRenderCallback = std::function<void(VkCommandBuffer)>;
    void setUIRenderCallback(std::function<void(VkCommandBuffer)> callback);

    void init(VulkanContext* context, SwapChain* swapChain);
    void cleanup();

    // Returns false if swap chain needs recreation
    bool drawFrame(Camera* camera);

    void waitIdle();

    // Called after swap chain is recreated
    void onSwapChainRecreated(SwapChain* newSwapChain);

    void submitMesh(Mesh* mesh, const glm::mat4& transform,
        const glm::vec3& color = glm::vec3(0.8f), bool selected = false);
    void clearSubmissions();

    // Get or create mesh in cache - creates GPU buffers if vertexData provided
    Mesh* getOrCreateMesh(uint64_t entityId, const void* vertexData, size_t vertexCount,
        const uint32_t* indexData, size_t indexCount);

    // Get mesh from cache without creating (returns nullptr if not found)
    Mesh* getMeshFromCache(uint64_t entityId);

    void removeMesh(uint64_t entityId);

    Grid* getGrid() { return grid; }
    VulkanContext* getContext() { return context; }

    // Get number of meshes in cache (for debugging)
    size_t getMeshCacheSize() const { return meshCache.size(); }

private:
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void createSceneObjects();

    // Pipeline management
    void createPipeline();
    void cleanupPipeline();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Camera* camera);
    void updateUniformBuffer(uint32_t currentImage, Camera* camera);

    std::function<void(VkCommandBuffer)> uiRenderCallback_;

    VulkanContext* context = nullptr;
    SwapChain* swapChain = nullptr;
    GraphicsPipeline* pipeline = nullptr;
    UniformBuffer* uniformBuffer = nullptr;

    Grid* grid = nullptr;
    std::unordered_map<uint64_t, Mesh*> meshCache;
    std::vector<RenderObject> renderQueue;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};