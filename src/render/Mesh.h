#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

class VulkanContext;

// Vertex structure for 3D meshes
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

// Line vertex (for grid and wireframe)
struct LineVertex {
    glm::vec3 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

// Edge structure
struct Edge {
    uint32_t v0, v1;
};

// Face structure (triangle)
struct Face {
    uint32_t v0, v1, v2;
    glm::vec3 normal;
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    void create(VulkanContext* context);
    void cleanup();

    // Set geometry data
    void setVertices(const std::vector<Vertex>& verts);
    void setIndices(const std::vector<uint32_t>& inds);

    // Binding for rendering
    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);
    void drawWireframe(VkCommandBuffer commandBuffer);

    // Transform
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    // Getters
    uint32_t getVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
    uint32_t getIndexCount() const { return static_cast<uint32_t>(indices.size()); }

    // Geometry data (public for manipulation)
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Edge> edges;
    std::vector<Face> faces;

private:
    void createVertexBuffer();
    void createIndexBuffer();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VulkanContext* context = nullptr;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
};