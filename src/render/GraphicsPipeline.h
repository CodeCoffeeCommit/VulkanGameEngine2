#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

class VulkanContext;
class SwapChain;
class UniformBuffer;

class GraphicsPipeline {
public:
    GraphicsPipeline();
    ~GraphicsPipeline();

    void init(VulkanContext* context, SwapChain* swapChain, UniformBuffer* uniformBuffer);
    void cleanup();

    // Mesh pipeline (triangles with lighting)
    VkPipeline getMeshPipeline() const { return meshPipeline; }
    VkPipelineLayout getMeshPipelineLayout() const { return meshPipelineLayout; }

    // Grid pipeline (lines)
    VkPipeline getGridPipeline() const { return gridPipeline; }
    VkPipelineLayout getGridPipelineLayout() const { return gridPipelineLayout; }

private:
    void createMeshPipeline();
    void createGridPipeline();

    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);

    VulkanContext* context = nullptr;
    SwapChain* swapChain = nullptr;
    UniformBuffer* uniformBuffer = nullptr;

    // Mesh rendering pipeline
    VkPipelineLayout meshPipelineLayout = VK_NULL_HANDLE;
    VkPipeline meshPipeline = VK_NULL_HANDLE;

    // Grid/line rendering pipeline
    VkPipelineLayout gridPipelineLayout = VK_NULL_HANDLE;
    VkPipeline gridPipeline = VK_NULL_HANDLE;
};