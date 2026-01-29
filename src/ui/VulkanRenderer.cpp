// src/ui/VulkanRenderer.cpp
// Vulkan implementation of LibreUI::Renderer

#include "VulkanRenderer.h"
#include <iostream>

namespace libre::ui {

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    void VulkanRenderer::init(VulkanContext* context, VkRenderPass renderPass) {
        impl_.init(context, renderPass);
        std::cout << "[OK] VulkanRenderer initialized (LibreUI adapter)\n";
    }

    void VulkanRenderer::cleanup() {
        impl_.cleanup();
    }

    // ========================================================================
    // FRAME LIFECYCLE
    // ========================================================================

    void VulkanRenderer::begin(float screenWidth, float screenHeight) {
        impl_.begin(screenWidth, screenHeight);
    }

    void VulkanRenderer::end() {
        if (commandBuffer_ == VK_NULL_HANDLE) {
            std::cerr << "[VulkanRenderer] WARNING: end() called without command buffer!\n";
            return;
        }
        impl_.end(commandBuffer_);
    }

    // ========================================================================
    // PRIMITIVE DRAWING
    // ========================================================================

    void VulkanRenderer::drawRect(const LibreUI::Rect& bounds, const LibreUI::Color& color) {
        impl_.drawRect(toInternal(bounds), toInternal(color));
    }

    void VulkanRenderer::drawRoundedRect(const LibreUI::Rect& bounds, const LibreUI::Color& color, float radius) {
        impl_.drawRoundedRect(toInternal(bounds), toInternal(color), radius);
    }

    void VulkanRenderer::drawRectOutline(const LibreUI::Rect& bounds, const LibreUI::Color& color, float thickness) {
        impl_.drawRectOutline(toInternal(bounds), toInternal(color), thickness);
    }

    // ========================================================================
    // TEXT RENDERING
    // ========================================================================

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
        return toLibreUI(impl_.measureText(text, size));
    }

    LibreUI::Vec2 VulkanRenderer::measureTextEx(const std::string& text, float size,
        const std::string& fontName, LibreUI::FontWeight weight) {
        return toLibreUI(impl_.measureTextEx(text, size, fontName, toInternal(weight)));
    }

    // ========================================================================
    // CLIPPING
    // ========================================================================

    void VulkanRenderer::pushClip(const LibreUI::Rect& bounds) {
        impl_.pushClip(toInternal(bounds));
    }

    void VulkanRenderer::popClip() {
        impl_.popClip();
    }

    // ========================================================================
    // STATE QUERIES
    // ========================================================================

    float VulkanRenderer::getScreenWidth() const {
        return impl_.getScreenWidth();
    }

    float VulkanRenderer::getScreenHeight() const {
        return impl_.getScreenHeight();
    }

} // namespace libre::ui