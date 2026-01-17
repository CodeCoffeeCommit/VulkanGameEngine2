#include "UIRenderer.h"
#include "../render/VulkanContext.h"
#include <cstring>
#include <stdexcept>
#include <array>


namespace libre::ui {

    // Simple 8x8 bitmap font (ASCII 32-127)
    // Each character is 8 pixels wide, stored as 8 bytes (one per row)
    static const unsigned char FONT_DATA[] = {
        // Space (32) through Tilde (126) - simplified bitmaps
        // This is a minimal font - replace with proper font atlas later
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Space
        0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00, // !
        0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00, // "
        0x6C,0xFE,0x6C,0x6C,0xFE,0x6C,0x00,0x00, // #
        0x18,0x7E,0x58,0x7C,0x1A,0x7E,0x18,0x00, // $
        0x66,0x6C,0x18,0x30,0x6C,0xC6,0x00,0x00, // %
        0x38,0x6C,0x38,0x76,0xCC,0xCC,0x76,0x00, // &
        0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00, // '
        0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00, // (
        0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00, // )
        0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00, // *
        0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00, // +
        0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30, // ,
        0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00, // -
        0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00, // .
        0x06,0x0C,0x18,0x30,0x60,0xC0,0x00,0x00, // /
        0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00, // 0
        0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00, // 1
        0x7C,0xC6,0x06,0x1C,0x30,0x66,0xFE,0x00, // 2
        0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00, // 3
        0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00, // 4
        0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00, // 5
        0x38,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00, // 6
        0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00, // 7
        0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00, // 8
        0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00, // 9
        0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00, // :
        0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30, // ;
        0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00, // <
        0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00, // =
        0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00, // >
        0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00, // ?
        0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C,0x00, // @
        0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00, // A
        0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00, // B
        0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00, // C
        0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00, // D
        0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00, // E
        0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00, // F
        0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3E,0x00, // G
        0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00, // H
        0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00, // I
        0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00, // J
        0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00, // K
        0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00, // L
        0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00, // M
        0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00, // N
        0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00, // O
        0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00, // P
        0x7C,0xC6,0xC6,0xC6,0xC6,0xCE,0x7C,0x0E, // Q
        0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00, // R
        0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00, // S
        0x7E,0x7E,0x5A,0x18,0x18,0x18,0x3C,0x00, // T
        0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00, // U
        0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00, // V
        0xC6,0xC6,0xC6,0xD6,0xD6,0xFE,0x6C,0x00, // W
        0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00, // X
        0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00, // Y
        0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00, // Z
        0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00, // [
        0xC0,0x60,0x30,0x18,0x0C,0x06,0x00,0x00, // backslash
        0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00, // ]
        0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00, // ^
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF, // _
        0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00, // `
        0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00, // a
        0xE0,0x60,0x7C,0x66,0x66,0x66,0xDC,0x00, // b
        0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00, // c
        0x1C,0x0C,0x7C,0xCC,0xCC,0xCC,0x76,0x00, // d
        0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00, // e
        0x3C,0x66,0x60,0xF8,0x60,0x60,0xF0,0x00, // f
        0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x78, // g
        0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00, // h
        0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00, // i
        0x06,0x00,0x0E,0x06,0x06,0x66,0x66,0x3C, // j
        0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00, // k
        0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00, // l
        0x00,0x00,0xEC,0xFE,0xD6,0xD6,0xD6,0x00, // m
        0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x00, // n
        0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00, // o
        0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0, // p
        0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E, // q
        0x00,0x00,0xDC,0x76,0x60,0x60,0xF0,0x00, // r
        0x00,0x00,0x7C,0xC0,0x7C,0x06,0x7C,0x00, // s
        0x30,0x30,0xFC,0x30,0x30,0x36,0x1C,0x00, // t
        0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00, // u
        0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x00, // v
        0x00,0x00,0xC6,0xD6,0xD6,0xFE,0x6C,0x00, // w
        0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00, // x
        0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x06,0x7C, // y
        0x00,0x00,0xFE,0x8C,0x18,0x32,0xFE,0x00, // z
        0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00, // {
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00, // |
        0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00, // }
        0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00, // ~
    };

    void UIRenderer::init(VulkanContext* context, VkRenderPass renderPass) {
        context_ = context;
        createPipeline(renderPass);
        createBuffers();
        createFontTexture();
    }

    void UIRenderer::cleanup() {
        if (!context_) return;
        VkDevice device = context_->getDevice();

        vkDestroySampler(device, fontSampler_, nullptr);
        vkDestroyImageView(device, fontView_, nullptr);
        vkDestroyImage(device, fontImage_, nullptr);
        vkFreeMemory(device, fontMemory_, nullptr);

        vkDestroyBuffer(device, vertexBuffer_, nullptr);
        vkFreeMemory(device, vertexMemory_, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool_, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout_, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        vkDestroyPipeline(device, pipeline_, nullptr);
    }

    void UIRenderer::begin(float screenWidth, float screenHeight) {
        screenWidth_ = screenWidth;
        screenHeight_ = screenHeight;
        vertices_.clear();
        clipStack_.clear();
    }

    void UIRenderer::end(VkCommandBuffer cmd) {
        if (vertices_.empty()) return;
        flushBatch(cmd);
    }

    void UIRenderer::drawRect(const Rect& bounds, const Color& color) {
        // Two triangles for a quad
        glm::vec4 c = color.toVec4();

        float x0 = bounds.x, y0 = bounds.y;
        float x1 = bounds.right(), y1 = bounds.bottom();

        // Clip if needed
        if (!clipStack_.empty()) {
            const Rect& clip = clipStack_.back();
            x0 = std::max(x0, clip.x);
            y0 = std::max(y0, clip.y);
            x1 = std::min(x1, clip.right());
            y1 = std::min(y1, clip.bottom());
            if (x1 <= x0 || y1 <= y0) return;
        }

        // Convert to NDC
        float nx0 = (x0 / screenWidth_) * 2.0f - 1.0f;
        float ny0 = (y0 / screenHeight_) * 2.0f - 1.0f;
        float nx1 = (x1 / screenWidth_) * 2.0f - 1.0f;
        float ny1 = (y1 / screenHeight_) * 2.0f - 1.0f;

        // Triangle 1
        vertices_.push_back({ {nx0, ny0}, {0, 0}, c });
        vertices_.push_back({ {nx1, ny0}, {1, 0}, c });
        vertices_.push_back({ {nx1, ny1}, {1, 1}, c });

        // Triangle 2
        vertices_.push_back({ {nx0, ny0}, {0, 0}, c });
        vertices_.push_back({ {nx1, ny1}, {1, 1}, c });
        vertices_.push_back({ {nx0, ny1}, {0, 1}, c });
    }

    void UIRenderer::drawRoundedRect(const Rect& bounds, const Color& color, float radius) {
        // For simplicity, just draw a regular rect for now
        // TODO: Implement rounded corners with more triangles or SDF
        drawRect(bounds, color);
    }

    void UIRenderer::drawRectOutline(const Rect& bounds, const Color& color, float thickness) {
        // Top
        drawRect({ bounds.x, bounds.y, bounds.w, thickness }, color);
        // Bottom
        drawRect({ bounds.x, bounds.bottom() - thickness, bounds.w, thickness }, color);
        // Left
        drawRect({ bounds.x, bounds.y, thickness, bounds.h }, color);
        // Right
        drawRect({ bounds.right() - thickness, bounds.y, thickness, bounds.h }, color);
    }

    void UIRenderer::drawText(const std::string& text, float x, float y, const Color& color, float size) {
        float scale = size / 8.0f;  // Font is 8 pixels tall
        float charWidth = 8.0f * scale;
        float charHeight = 8.0f * scale;

        float cursorX = x;
        glm::vec4 c = color.toVec4();

        for (char ch : text) {
            if (ch < 32 || ch > 126) ch = '?';
            int charIndex = ch - 32;

            // UV coordinates in the font atlas
            // Font atlas is 16x6 characters (96 total), each 8x8 pixels
            // Atlas size: 128x48 pixels
            int atlasX = charIndex % 16;
            int atlasY = charIndex / 16;

            float u0 = atlasX / 16.0f;
            float v0 = atlasY / 6.0f;
            float u1 = (atlasX + 1) / 16.0f;
            float v1 = (atlasY + 1) / 6.0f;

            float x0 = cursorX, y0 = y;
            float x1 = cursorX + charWidth, y1 = y + charHeight;

            // Convert to NDC
            float nx0 = (x0 / screenWidth_) * 2.0f - 1.0f;
            float ny0 = (y0 / screenHeight_) * 2.0f - 1.0f;
            float nx1 = (x1 / screenWidth_) * 2.0f - 1.0f;
            float ny1 = (y1 / screenHeight_) * 2.0f - 1.0f;

            // Mark as text (use negative UV to signal text rendering in shader)
            vertices_.push_back({ {nx0, ny0}, {u0, v0}, c });
            vertices_.push_back({ {nx1, ny0}, {u1, v0}, c });
            vertices_.push_back({ {nx1, ny1}, {u1, v1}, c });

            vertices_.push_back({ {nx0, ny0}, {u0, v0}, c });
            vertices_.push_back({ {nx1, ny1}, {u1, v1}, c });
            vertices_.push_back({ {nx0, ny1}, {u0, v1}, c });

            cursorX += charWidth;
        }
    }

    Vec2 UIRenderer::measureText(const std::string& text, float size) {
        float scale = size / 8.0f;
        return { text.length() * 8.0f * scale, 8.0f * scale };
    }

    void UIRenderer::pushClip(const Rect& bounds) {
        if (clipStack_.empty()) {
            clipStack_.push_back(bounds);
        }
        else {
            // Intersect with current clip
            const Rect& current = clipStack_.back();
            Rect clipped;
            clipped.x = std::max(bounds.x, current.x);
            clipped.y = std::max(bounds.y, current.y);
            clipped.w = std::min(bounds.right(), current.right()) - clipped.x;
            clipped.h = std::min(bounds.bottom(), current.bottom()) - clipped.y;
            clipStack_.push_back(clipped);
        }
    }

    void UIRenderer::popClip() {
        if (!clipStack_.empty()) {
            clipStack_.pop_back();
        }
    }

    void UIRenderer::flushBatch(VkCommandBuffer cmd) {
        if (vertices_.empty()) return;

        // Upload vertices
        void* data;
        vkMapMemory(context_->getDevice(), vertexMemory_, 0,
            vertices_.size() * sizeof(UIVertex), 0, &data);
        memcpy(data, vertices_.data(), vertices_.size() * sizeof(UIVertex));
        vkUnmapMemory(context_->getDevice(), vertexMemory_);

        // Bind and draw
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout_, 0, 1, &descriptorSet_, 0, nullptr);

        VkBuffer buffers[] = { vertexBuffer_ };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdDraw(cmd, static_cast<uint32_t>(vertices_.size()), 1, 0, 0);
    }

    void UIRenderer::createPipeline(VkRenderPass renderPass) {
        VkDevice device = context_->getDevice();

        // Descriptor set layout for font texture
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout_);

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout_);

        // Note: Shader modules should be loaded from files
        // For now, we assume shaders exist at shaders/compiled/ui.vert.spv and ui.frag.spv
    }

    void UIRenderer::createBuffers() {
        VkDevice device = context_->getDevice();

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MAX_VERTICES * sizeof(UIVertex);
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer_);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, vertexBuffer_, &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = context_->findMemoryType(
            memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &allocInfo, nullptr, &vertexMemory_);
        vkBindBufferMemory(device, vertexBuffer_, vertexMemory_, 0);
    }

    void UIRenderer::createFontTexture() {
        // Create a 128x48 font atlas from the bitmap data
        const int atlasWidth = 128;
        const int atlasHeight = 48;
        std::vector<uint8_t> pixels(atlasWidth * atlasHeight * 4, 0);

        // Convert bitmap font to RGBA
        for (int charIndex = 0; charIndex < 95; charIndex++) {
            int atlasX = (charIndex % 16) * 8;
            int atlasY = (charIndex / 16) * 8;

            for (int row = 0; row < 8; row++) {
                uint8_t rowData = FONT_DATA[charIndex * 8 + row];
                for (int col = 0; col < 8; col++) {
                    bool set = (rowData >> (7 - col)) & 1;
                    int px = atlasX + col;
                    int py = atlasY + row;
                    int idx = (py * atlasWidth + px) * 4;
                    pixels[idx + 0] = 255;
                    pixels[idx + 1] = 255;
                    pixels[idx + 2] = 255;
                    pixels[idx + 3] = set ? 255 : 0;
                }
            }
        }

        // Create Vulkan image (simplified - in real code, use staging buffer)
        VkDevice device = context_->getDevice();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { atlasWidth, atlasHeight, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        vkCreateImage(device, &imageInfo, nullptr, &fontImage_);

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, fontImage_, &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = context_->findMemoryType(
            memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &allocInfo, nullptr, &fontMemory_);
        vkBindImageMemory(device, fontImage_, fontMemory_, 0);

        // Copy pixel data
        void* data;
        vkMapMemory(device, fontMemory_, 0, memReq.size, 0, &data);
        memcpy(data, pixels.data(), pixels.size());
        vkUnmapMemory(device, fontMemory_);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = fontImage_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &fontView_);

        // Create sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(device, &samplerInfo, nullptr, &fontSampler_);

        // Create descriptor pool and set
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_);

        VkDescriptorSetAllocateInfo descAllocInfo{};
        descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool = descriptorPool_;
        descAllocInfo.descriptorSetCount = 1;
        descAllocInfo.pSetLayouts = &descriptorSetLayout_;
        vkAllocateDescriptorSets(device, &descAllocInfo, &descriptorSet_);

        VkDescriptorImageInfo imageDescInfo{};
        imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageDescInfo.imageView = fontView_;
        imageDescInfo.sampler = fontSampler_;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet_;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageDescInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

} // namespace libre::ui