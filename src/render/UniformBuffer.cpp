#include "UniformBuffer.h"
#include "VulkanContext.h"
#include <stdexcept>
#include <cstring>

UniformBuffer::UniformBuffer() {}

UniformBuffer::~UniformBuffer() {}

void UniformBuffer::create(VulkanContext* ctx, uint32_t count) {
    this->context = ctx;

    createDescriptorSetLayout();
    createUniformBuffers(count);
    createDescriptorPool(count);
    createDescriptorSets(count);
}

void UniformBuffer::cleanup() {
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        vkDestroyBuffer(context->getDevice(), uniformBuffers[i], nullptr);
        vkFreeMemory(context->getDevice(), uniformBuffersMemory[i], nullptr);
    }

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(context->getDevice(), descriptorPool, nullptr);
    }

    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context->getDevice(), descriptorSetLayout, nullptr);
    }
}

void UniformBuffer::update(uint32_t index, const UniformBufferObject& ubo) {
    memcpy(uniformBuffersMapped[index], &ubo, sizeof(ubo));
}

void UniformBuffer::createUniformBuffers(uint32_t count) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(count);
    uniformBuffersMemory.resize(count);
    uniformBuffersMapped.resize(count);

    for (size_t i = 0; i < count; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(context->getDevice(), uniformBuffersMemory[i], 0, bufferSize, 0,
            &uniformBuffersMapped[i]);
    }
}

void UniformBuffer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(context->getDevice(), &layoutInfo, nullptr,
        &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void UniformBuffer::createDescriptorPool(uint32_t count) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = count;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = count;

    if (vkCreateDescriptorPool(context->getDevice(), &poolInfo, nullptr,
        &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void UniformBuffer::createDescriptorSets(uint32_t count) {
    std::vector<VkDescriptorSetLayout> layouts(count, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(count);
    if (vkAllocateDescriptorSets(context->getDevice(), &allocInfo,
        descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < count; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(context->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }
}

void UniformBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer,
    VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context->getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context->getDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context->getDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(context->getDevice(), buffer, bufferMemory, 0);
}

uint32_t UniformBuffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context->getPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}