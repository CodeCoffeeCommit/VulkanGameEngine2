#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

class VulkanContext;
struct LineVertex;

class Grid {
public:
    Grid();
    ~Grid();

    void create(VulkanContext* context, float size = 10.0f, int divisions = 10);
    void cleanup();

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

    uint32_t getVertexCount() const { return vertexCount; }

private:
    void createVertexBuffer();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VulkanContext* context = nullptr;

    std::vector<LineVertex> vertices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
};