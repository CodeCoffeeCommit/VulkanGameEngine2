// src/ui/VulkanRenderer.h
// Vulkan implementation of LibreUI::Renderer
// This bridges LibreUI widgets to the Vulkan-based UIRenderer

#pragma once

#include <LibreUI/Renderer.h>
#include "UIRenderer.h"
#include <vulkan/vulkan.h>

class VulkanContext;

namespace libre::ui {

    // ============================================================================
    // VULKAN RENDERER - Implements LibreUI::Renderer using Vulkan
    // ============================================================================

    class VulkanRenderer : public LibreUI::Renderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() override = default;

        // --------------------------------------------------------------------
        // Vulkan-specific initialization (NOT part of LibreUI interface)
        // --------------------------------------------------------------------

        void init(VulkanContext* context, VkRenderPass renderPass);
        void cleanup();

        // Must be called before end() each frame
        void setCommandBuffer(VkCommandBuffer cmd) { commandBuffer_ = cmd; }

        // Access underlying renderer for advanced usage
        UIRenderer& getImpl() { return impl_; }

        // --------------------------------------------------------------------
        // LibreUI::Renderer interface implementation
        // --------------------------------------------------------------------

        void begin(float screenWidth, float screenHeight) override;
        void end() override;

        void drawRect(const LibreUI::Rect& bounds, const LibreUI::Color& color) override;
        void drawRoundedRect(const LibreUI::Rect& bounds, const LibreUI::Color& color, float radius) override;
        void drawRectOutline(const LibreUI::Rect& bounds, const LibreUI::Color& color, float thickness = 1.0f) override;

        void drawText(const std::string& text, float x, float y,
            const LibreUI::Color& color, float size = 13.0f) override;

        void drawTextEx(const std::string& text, float x, float y,
            const LibreUI::Color& color, float size,
            const std::string& fontName,
            LibreUI::FontWeight weight = LibreUI::FontWeight::Regular) override;

        LibreUI::Vec2 measureText(const std::string& text, float size = 13.0f) override;

        LibreUI::Vec2 measureTextEx(const std::string& text, float size,
            const std::string& fontName,
            LibreUI::FontWeight weight = LibreUI::FontWeight::Regular) override;

        void pushClip(const LibreUI::Rect& bounds) override;
        void popClip() override;

        float getScreenWidth() const override;
        float getScreenHeight() const override;

    private:
        // Type aliases to avoid namespace ambiguity
        using InternalFontWeight = ::libre::ui::FontWeight;
        using InternalRect = ::libre::ui::Rect;
        using InternalColor = ::libre::ui::Color;
        using InternalVec2 = ::libre::ui::Vec2;
        
        
        // --------------------------------------------------------------------
        // Type conversion helpers
        // --------------------------------------------------------------------

        static Rect toInternal(const LibreUI::Rect& r) {
            return Rect(r.x, r.y, r.w, r.h);
        }

        static Color toInternal(const LibreUI::Color& c) {
            return Color(c.r, c.g, c.b, c.a);
        }

        static LibreUI::Vec2 toLibreUI(const Vec2& v) {
            return LibreUI::Vec2(v.x, v.y);
        }

        static InternalFontWeight toInternal(LibreUI::FontWeight w) {
            return static_cast<InternalFontWeight>(static_cast<int>(w));
        }

        // --------------------------------------------------------------------
        // Members
        // --------------------------------------------------------------------

        UIRenderer impl_;
        VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    };

} // namespace libre::ui