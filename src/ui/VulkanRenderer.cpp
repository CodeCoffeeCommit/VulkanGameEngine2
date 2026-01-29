// src/ui/VulkanRenderer.cpp
// Minimal test version

#include "VulkanRenderer.h"

namespace libre::ui {

    void VulkanRenderer::init(VulkanContext* context, VkRenderPass renderPass) {
        impl_.init(context, renderPass);
    }

    void VulkanRenderer::cleanup() {
        impl_.cleanup();
    }

    void VulkanRenderer::begin(float screenWidth, float screenHeight) {
        impl_.begin(screenWidth, screenHeight);
    }

    void VulkanRenderer::end() {
        if (commandBuffer_ != VK_NULL_HANDLE) {
            impl_.end(commandBuffer_);
        }
    }

    void VulkanRenderer::drawRect(const LibreUI::Rect& bounds, const LibreUI::Color& color) {
        impl_.drawRect(toInternal(bounds), toInternal(color));
    }

    void VulkanRenderer::drawRoundedRect(const LibreUI::Rect& bounds, const LibreUI::Color& color, float radius) {
        impl_.drawRoundedRect(toInternal(bounds), toInternal(color), radius);
    }

    void VulkanRenderer::drawRectOutline(const LibreUI::Rect& bounds, const LibreUI::Color& color, float thickness) {
        impl_.drawRectOutline(toInternal(bounds), toInternal(color), thickness);
    }

    void VulkanRenderer::drawText(const std::string& text, float x, float y,
        const LibreUI::Color& color, float size) {
        impl_.drawText(text, x, y, toInternal(color), size);
    }

    void VulkanRenderer::drawTextEx(const std::string& text, float x, float y,
        const LibreUI::Color& color, float size,
        const std::string& fontName, LibreUI::FontWeight weight) {
        impl_.drawTextEx(text, x, y, toInternal(color), size, fontName, toInternal(weight));
    }

    LibreUI::Vec2 VulkanRenderer::measureText(const std::string& text, float size) {
        Vec2 v = impl_.measureText(text, size);
        return LibreUI::Vec2(v.x, v.y);
    }

    LibreUI::Vec2 VulkanRenderer::measureTextEx(const std::string& text, float size,
        const std::string& fontName, LibreUI::FontWeight weight) {
        Vec2 v = impl_.measureTextEx(text, size, fontName, toInternal(weight));
        return LibreUI::Vec2(v.x, v.y);
    }

    void VulkanRenderer::pushClip(const LibreUI::Rect& bounds) {
        impl_.pushClip(toInternal(bounds));
    }

    void VulkanRenderer::popClip() {
        impl_.popClip();
    }

    float VulkanRenderer::getScreenWidth() const {
        return impl_.getScreenWidth();
    }

    float VulkanRenderer::getScreenHeight() const {
        return impl_.getScreenHeight();
    }

} // namespace libre::ui