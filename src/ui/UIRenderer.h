// src/ui/UIRenderer.h
#pragma once

#include "Core.h"
#include "Theme.h"
#include "FontSystem.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VulkanContext;  // Forward declaration (outside namespace)

namespace libre::ui {

    // Simple vertex for UI quads
    struct UIVertex {
        glm::vec2 pos;
        glm::vec2 uv;
        glm::vec4 color;
    };

    class UIRenderer {
    public:
        void init(VulkanContext* context, VkRenderPass renderPass);
        void cleanup();

        // Call at start of frame
        void begin(float screenWidth, float screenHeight);

        // Call at end of frame to submit draws
        void end(VkCommandBuffer cmd);

        // ========================================================================
        // DRAWING PRIMITIVES
        // ========================================================================

        // Solid rectangle
        void drawRect(const Rect& bounds, const Color& color);

        // Rounded rectangle
        void drawRoundedRect(const Rect& bounds, const Color& color, float radius);

        // Rectangle outline
        void drawRectOutline(const Rect& bounds, const Color& color, float thickness = 1.0f);

        // ========================================================================
        // TEXT RENDERING (FreeType-based with Blender-style rendering)
        // ========================================================================

        // Basic text drawing - uses default font
        void drawText(const std::string& text, float x, float y, const Color& color, float size = 13.0f);

        // Advanced text drawing - specify font and weight
        void drawTextEx(const std::string& text, float x, float y, const Color& color, float size,
            const std::string& fontName, FontWeight weight = FontWeight::Regular);

        // Measure text dimensions - uses default font
        Vec2 measureText(const std::string& text, float size = 13.0f);

        // Measure text dimensions - specify font and weight
        Vec2 measureTextEx(const std::string& text, float size,
            const std::string& fontName, FontWeight weight = FontWeight::Regular);

        // ========================================================================
        // CLIPPING
        // ========================================================================

        void pushClip(const Rect& bounds);
        void popClip();

        // ========================================================================
        // STATE
        // ========================================================================

        float getScreenWidth() const { return screenWidth_; }
        float getScreenHeight() const { return screenHeight_; }

        // Access to font system for advanced usage
        FontSystem& getFontSystem() { return FontSystem::instance(); }

    private:
        void createPipeline(VkRenderPass renderPass);
        void createBuffers();
        void createDescriptorResources();
        void updateFontDescriptor();
        void flushBatch(VkCommandBuffer cmd);

        VulkanContext* context_ = nullptr;

        // Pipeline
        VkPipeline pipeline_ = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

        // Buffers
        VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory_ = VK_NULL_HANDLE;
        static constexpr size_t MAX_VERTICES = 65536;

        // Batching
        std::vector<UIVertex> vertices_;
        std::vector<Rect> clipStack_;

        float screenWidth_ = 0;
        float screenHeight_ = 0;

        // Font system initialized flag
        bool fontSystemInitialized_ = false;
    };

} // namespace libre::ui