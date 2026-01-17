#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

class VulkanContext;

// Scene-wide uniform data (constant for entire frame)
struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 lightDir;
    alignas(4)  float _pad1;
    alignas(16) glm::vec3 viewPos;
    alignas(4)  float _pad2;
};

// Per-object data sent via push constants
struct PushConstants {
    glm::mat4 model;
};

class UniformBuffer {
public:
    UniformBuffer();
    ~UniformBuffer();

    void create(VulkanContext* context, uint32_t frameCount);
    void cleanup();

    void update(uint32_t frameIndex, const UniformBufferObject& ubo);

    VkBuffer getBuffer(uint32_t frameIndex) const { return uniformBuffers[frameIndex]; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const { return descriptorSets[frameIndex]; }

private:
    void createUniformBuffers(uint32_t count);
    void createDescriptorSetLayout();
    void createDescriptorPool(uint32_t count);
    void createDescriptorSets(uint32_t count);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VulkanContext* context = nullptr;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
};