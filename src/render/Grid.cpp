#include "Grid.h"
#include "Mesh.h"  // For LineVertex
#include "VulkanContext.h"
#include <stdexcept>
#include <cstring>

Grid::Grid() {}

Grid::~Grid() {}

void Grid::create(VulkanContext* ctx, float size, int divisions) {
    this->context = ctx;

    // Blender-style colors
    glm::vec3 gridColor(0.25f, 0.25f, 0.25f);      // Gray grid lines
    glm::vec3 xAxisColor(0.5f, 0.15f, 0.15f);      // Red X axis (darker)
    glm::vec3 yAxisColor(0.15f, 0.5f, 0.15f);      // Green Y axis (darker)
    glm::vec3 centerLineColor(0.35f, 0.35f, 0.35f); // Slightly brighter center

    float halfSize = size;
    float step = (size * 2.0f) / divisions;

    vertices.clear();

    // Generate grid lines on XZ plane
    for (int i = 0; i <= divisions; i++) {
        float pos = -halfSize + i * step;

        // Determine line color
        glm::vec3 colorX = gridColor;
        glm::vec3 colorZ = gridColor;

        if (i == divisions / 2) {
            // Center lines (axes)
            colorX = yAxisColor;  // Line along Z is Y-axis indicator
            colorZ = xAxisColor;  // Line along X is X-axis indicator
        }

        // Lines parallel to X axis (along Z)
        vertices.push_back({ {-halfSize, 0.0f, pos}, colorX });
        vertices.push_back({ {halfSize, 0.0f, pos}, colorX });

        // Lines parallel to Z axis (along X)
        vertices.push_back({ {pos, 0.0f, -halfSize}, colorZ });
        vertices.push_back({ {pos, 0.0f, halfSize}, colorZ });
    }

    // Add Y axis line (vertical)
    vertices.push_back({ {0.0f, 0.0f, 0.0f}, glm::vec3(0.15f, 0.15f, 0.5f) });  // Blue
    vertices.push_back({ {0.0f, halfSize, 0.0f}, glm::vec3(0.15f, 0.15f, 0.5f) });

    vertexCount = static_cast<uint32_t>(vertices.size());

    createVertexBuffer();
}

void Grid::cleanup() {
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(context->getDevice(), vertexBuffer, nullptr);
        vkFreeMemory(context->getDevice(), vertexBufferMemory, nullptr);
    }
}

void Grid::bind(VkCommandBuffer commandBuffer) {
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

void Grid::draw(VkCommandBuffer commandBuffer) {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

void Grid::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBuffer, vertexBufferMemory);

    void* data;
    vkMapMemory(context->getDevice(), vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(context->getDevice(), vertexBufferMemory);
}

void Grid::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer,
    VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context->getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create grid buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context->getDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context->getDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate grid buffer memory!");
    }

    vkBindBufferMemory(context->getDevice(), buffer, bufferMemory, 0);
}

uint32_t Grid::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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