#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class VulkanContext;

class SwapChain {
public:
    SwapChain();
    ~SwapChain();

    void init(VulkanContext* context, GLFWwindow* window);
    void cleanup();
    void recreate(GLFWwindow* window);

    // Getters
    VkSwapchainKHR getSwapChain() const { return swapChain; }
    VkFormat getImageFormat() const { return swapChainImageFormat; }
    VkExtent2D getExtent() const { return swapChainExtent; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapChainFramebuffers; }
    VkRenderPass getRenderPass() const { return renderPass; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(swapChainImages.size()); }

private:
    void createSwapChain(GLFWwindow* window);
    void createImageViews();
    void createRenderPass();
    void createDepthResources();
    void createFramebuffers();
    void cleanupSwapChain();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
        VkImageTiling tiling, VkFormatFeatureFlags features);
    void createImage(uint32_t width, uint32_t height, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image,
        VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VulkanContext* context = nullptr;

    // Swap chain - INITIALIZED
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent = { 0, 0 };

    // Depth buffer - INITIALIZED
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    // Render pass and framebuffers
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers;
};