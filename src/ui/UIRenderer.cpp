#include "UIRenderer.h"
#include "../render/VulkanContext.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <array>
#include <algorithm>

namespace {
    // 8x8 pixel font data - each character is 8 bytes (one byte per row)
    // Covers ASCII 32 (space) to 126 (~)
    const unsigned char FONT_8X8[95][8] = {
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 (space)
        {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00}, // 33 !
        {0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00}, // 34 "
        {0x6C,0xFE,0x6C,0x6C,0xFE,0x6C,0x00,0x00}, // 35 #
        {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // 36 $
        {0x62,0x66,0x0C,0x18,0x30,0x66,0x46,0x00}, // 37 %
        {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // 38 &
        {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // 39 '
        {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 40 (
        {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 )
        {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 *
        {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 43 +
        {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ,
        {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 45 -
        {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 .
        {0x02,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, // 47 /
        {0x3C,0x66,0x6E,0x7E,0x76,0x66,0x3C,0x00}, // 48 0
        {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 49 1
        {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, // 50 2
        {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 51 3
        {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00}, // 52 4
        {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 53 5
        {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 54 6
        {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00}, // 55 7
        {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 56 8
        {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00}, // 57 9
        {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // 58 :
        {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, // 59 ;
        {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // 60 <
        {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // 61 =
        {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // 62 >
        {0x3C,0x66,0x0C,0x18,0x18,0x00,0x18,0x00}, // 63 ?
        {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00}, // 64 @
        {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 65 A
        {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 66 B
        {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 67 C
        {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 68 D
        {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // 69 E
        {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // 70 F
        {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00}, // 71 G
        {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 72 H
        {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00}, // 73 I
        {0x3E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, // 74 J
        {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 75 K
        {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 76 L
        {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 77 M
        {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 78 N
        {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 79 O
        {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 80 P
        {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, // 81 Q
        {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, // 82 R
        {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 83 S
        {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 T
        {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 85 U
        {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 86 V
        {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 W
        {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 88 X
        {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // 89 Y
        {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // 90 Z
        {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 91 [
        {0x40,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, // 92 backslash
        {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 93 ]
        {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00}, // 94 ^
        {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00}, // 95 _
        {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // 96 `
        {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // 97 a
        {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // 98 b
        {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, // 99 c
        {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // 100 d
        {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // 101 e
        {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00}, // 102 f
        {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}, // 103 g
        {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // 104 h
        {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 105 i
        {0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x6C,0x38}, // 106 j
        {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // 107 k
        {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 108 l
        {0x00,0x00,0x76,0x7F,0x6B,0x6B,0x63,0x00}, // 109 m
        {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // 110 n
        {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // 111 o
        {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // 112 p
        {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, // 113 q
        {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, // 114 r
        {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // 115 s
        {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00}, // 116 t
        {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // 117 u
        {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // 118 v
        {0x00,0x00,0x63,0x6B,0x6B,0x7F,0x36,0x00}, // 119 w
        {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // 120 x
        {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C}, // 121 y
        {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // 122 z
        {0x0C,0x18,0x18,0x70,0x18,0x18,0x0C,0x00}, // 123 {
        {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 124 |
        {0x30,0x18,0x18,0x0E,0x18,0x18,0x30,0x00}, // 125 }
        {0x31,0x6B,0x46,0x00,0x00,0x00,0x00,0x00}, // 126 ~
    };

    // Generate a font texture atlas (128x64 pixels, 16x8 grid of 8x8 chars)
    void generateFontAtlas(std::vector<unsigned char>& pixels, int& width, int& height) {
        width = 128;   // 16 chars * 8 pixels
        height = 64;   // 8 rows * 8 pixels (we only need 6 rows for 95 chars)
        pixels.resize(width * height, 0);

        for (int charIdx = 0; charIdx < 95; charIdx++) {
            int gridX = charIdx % 16;
            int gridY = charIdx / 16;

            for (int row = 0; row < 8; row++) {
                unsigned char rowData = FONT_8X8[charIdx][row];
                for (int col = 0; col < 8; col++) {
                    bool pixel = (rowData >> (7 - col)) & 1;
                    int x = gridX * 8 + col;
                    int y = gridY * 8 + row;
                    pixels[y * width + x] = pixel ? 255 : 0;
                }
            }
        }
    }
}


namespace libre::ui {

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    void UIRenderer::init(VulkanContext* context, VkRenderPass renderPass) {
        context_ = context;
        createPipeline(renderPass);
        createBuffers();
        createFontTexture();
        std::cout << "[OK] UIRenderer initialized" << std::endl;
    }

    void UIRenderer::cleanup() {
        if (!context_) return;
        VkDevice device = context_->getDevice();

        vkDeviceWaitIdle(device);

        if (fontSampler_ != VK_NULL_HANDLE) {
            vkDestroySampler(device, fontSampler_, nullptr);
            fontSampler_ = VK_NULL_HANDLE;
        }
        if (fontView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device, fontView_, nullptr);
            fontView_ = VK_NULL_HANDLE;
        }
        if (fontImage_ != VK_NULL_HANDLE) {
            vkDestroyImage(device, fontImage_, nullptr);
            fontImage_ = VK_NULL_HANDLE;
        }
        if (fontMemory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device, fontMemory_, nullptr);
            fontMemory_ = VK_NULL_HANDLE;
        }
        if (vertexBuffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vertexBuffer_, nullptr);
            vertexBuffer_ = VK_NULL_HANDLE;
        }
        if (vertexMemory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device, vertexMemory_, nullptr);
            vertexMemory_ = VK_NULL_HANDLE;
        }
        if (descriptorPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
        }
        if (descriptorSetLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout_, nullptr);
            descriptorSetLayout_ = VK_NULL_HANDLE;
        }
        if (pipelineLayout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
            pipelineLayout_ = VK_NULL_HANDLE;
        }
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }

        context_ = nullptr;
        std::cout << "[OK] UIRenderer cleaned up" << std::endl;
    }

    void UIRenderer::begin(float screenWidth, float screenHeight) {
        screenWidth_ = screenWidth;
        screenHeight_ = screenHeight;
        vertices_.clear();
        clipStack_.clear();
    }

    void UIRenderer::end(VkCommandBuffer cmd) {
        if (vertices_.empty()) {
            return;
        }
        flushBatch(cmd);
    }

    void UIRenderer::drawRect(const Rect& bounds, const Color& color) {
        glm::vec4 c = color.toVec4();

        float x0 = bounds.x, y0 = bounds.y;
        float x1 = bounds.right(), y1 = bounds.bottom();

        if (!clipStack_.empty()) {
            const Rect& clip = clipStack_.back();
            x0 = std::max(x0, clip.x);
            y0 = std::max(y0, clip.y);
            x1 = std::min(x1, clip.right());
            y1 = std::min(y1, clip.bottom());
            if (x1 <= x0 || y1 <= y0) return;
        }

        float nx0 = (x0 / screenWidth_) * 2.0f - 1.0f;
        float ny0 = (y0 / screenHeight_) * 2.0f - 1.0f;
        float nx1 = (x1 / screenWidth_) * 2.0f - 1.0f;
        float ny1 = (y1 / screenHeight_) * 2.0f - 1.0f;

        vertices_.push_back({ {nx0, ny0}, {-1, -1}, c });
        vertices_.push_back({ {nx1, ny0}, {-1, -1}, c });
        vertices_.push_back({ {nx1, ny1}, {-1, -1}, c });

        vertices_.push_back({ {nx0, ny0}, {-1, -1}, c });
        vertices_.push_back({ {nx1, ny1}, {-1, -1}, c });
        vertices_.push_back({ {nx0, ny1}, {-1, -1}, c });
    }

    void UIRenderer::drawRoundedRect(const Rect& bounds, const Color& color, float radius) {
        drawRect(bounds, color);
    }

    void UIRenderer::drawRectOutline(const Rect& bounds, const Color& color, float thickness) {
        drawRect({ bounds.x, bounds.y, bounds.w, thickness }, color);
        drawRect({ bounds.x, bounds.bottom() - thickness, bounds.w, thickness }, color);
        drawRect({ bounds.x, bounds.y, thickness, bounds.h }, color);
        drawRect({ bounds.right() - thickness, bounds.y, thickness, bounds.h }, color);
    }

    void UIRenderer::drawText(const std::string& text, float x, float y, const Color& color, float size) {
        float scale = size / 8.0f;
        float charWidth = 8.0f * scale;
        float charHeight = 8.0f * scale;

        float cursorX = x;
        glm::vec4 c = color.toVec4();

        for (char ch : text) {
            if (ch < 32 || ch > 126) ch = '?';
            int charIndex = ch - 32;

            int atlasX = charIndex % 16;
            int atlasY = charIndex / 16;

            // UV coordinates for 128x64 atlas (16 columns, 8 rows)
            float u0 = atlasX / 16.0f;
            float v0 = atlasY / 8.0f;
            float u1 = (atlasX + 1) / 16.0f;
            float v1 = (atlasY + 1) / 8.0f;

            float x0 = cursorX, y0 = y;
            float x1 = cursorX + charWidth, y1 = y + charHeight;

            float nx0 = (x0 / screenWidth_) * 2.0f - 1.0f;
            float ny0 = (y0 / screenHeight_) * 2.0f - 1.0f;
            float nx1 = (x1 / screenWidth_) * 2.0f - 1.0f;
            float ny1 = (y1 / screenHeight_) * 2.0f - 1.0f;

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
        std::cout << "[flushBatch] Called with " << vertices_.size() << " vertices" << std::endl;

        if (vertices_.empty()) {
            return;
        }

        if (pipeline_ == VK_NULL_HANDLE) {
            std::cerr << "[UIRenderer] ERROR: Pipeline is null!" << std::endl;
            return;
        }

        if (vertexBuffer_ == VK_NULL_HANDLE || vertexMemory_ == VK_NULL_HANDLE) {
            std::cerr << "[UIRenderer] ERROR: Vertex buffer not initialized!" << std::endl;
            return;
        }

        // Upload vertex data
        void* data;
        VkResult mapResult = vkMapMemory(context_->getDevice(), vertexMemory_, 0,
            vertices_.size() * sizeof(UIVertex), 0, &data);
        if (mapResult != VK_SUCCESS) {
            std::cerr << "[UIRenderer] ERROR: Failed to map vertex memory!" << std::endl;
            return;
        }
        memcpy(data, vertices_.data(), vertices_.size() * sizeof(UIVertex));
        vkUnmapMemory(context_->getDevice(), vertexMemory_);

        // Bind pipeline and descriptors
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout_, 0, 1, &descriptorSet_, 0, nullptr);

        // Bind vertex buffer and draw
        VkBuffer buffers[] = { vertexBuffer_ };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdDraw(cmd, static_cast<uint32_t>(vertices_.size()), 1, 0, 0);
    }

    void UIRenderer::createPipeline(VkRenderPass renderPass) {
        VkDevice device = context_->getDevice();

        // Load shaders
        std::vector<char> vertShaderCode;
        std::vector<char> fragShaderCode;

        try {
            vertShaderCode = readFile("shaders/compiled/ui.vert.spv");
            fragShaderCode = readFile("shaders/compiled/ui.frag.spv");
        }
        catch (const std::exception& e) {
            std::cerr << "[UIRenderer] Failed to load shaders: " << e.what() << std::endl;
            return;
        }

        // Create shader modules
        VkShaderModuleCreateInfo vertCreateInfo{};
        vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertCreateInfo.codeSize = vertShaderCode.size();
        vertCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());

        VkShaderModule vertShaderModule;
        if (vkCreateShaderModule(device, &vertCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create vertex shader module!" << std::endl;
            return;
        }

        VkShaderModuleCreateInfo fragCreateInfo{};
        fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragCreateInfo.codeSize = fragShaderCode.size();
        fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());

        VkShaderModule fragShaderModule;
        if (vkCreateShaderModule(device, &fragCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create fragment shader module!" << std::endl;
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            return;
        }

        // Shader stages
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Vertex input
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(UIVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(UIVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(UIVertex, uv);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(UIVertex, color);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport state (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Depth stencil - disable depth testing for UI
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // Color blending with alpha
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Descriptor set layout
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &samplerBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create descriptor set layout!" << std::endl;
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            return;
        }

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create pipeline layout!" << std::endl;
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            return;
        }

        // Create graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout_;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_);

        // Cleanup shader modules
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

        if (result != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create graphics pipeline! Error: " << result << std::endl;
            return;
        }

        std::cout << "[OK] UI pipeline created" << std::endl;
    }

    void UIRenderer::createBuffers() {
        VkDevice device = context_->getDevice();

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MAX_VERTICES * sizeof(UIVertex);
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create vertex buffer!" << std::endl;
            return;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, vertexBuffer_, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = context_->findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexMemory_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to allocate vertex buffer memory!" << std::endl;
            return;
        }

        vkBindBufferMemory(device, vertexBuffer_, vertexMemory_, 0);
        std::cout << "[OK] UI vertex buffer created" << std::endl;
    }

    void UIRenderer::createFontTexture() {
        VkDevice device = context_->getDevice();

        // Generate font atlas
        std::vector<unsigned char> pixels;
        int width, height;
        generateFontAtlas(pixels, width, height);

        VkDeviceSize imageSize = width * height;

        // --- Create staging buffer ---
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = imageSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &stagingBufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create staging buffer!" << std::endl;
            return;
        }

        VkMemoryRequirements stagingMemReq;
        vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingMemReq);

        VkMemoryAllocateInfo stagingAllocInfo{};
        stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        stagingAllocInfo.allocationSize = stagingMemReq.size;
        stagingAllocInfo.memoryTypeIndex = context_->findMemoryType(stagingMemReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &stagingAllocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to allocate staging buffer memory!" << std::endl;
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            return;
        }

        vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

        // Copy pixel data to staging buffer
        void* data;
        vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels.data(), imageSize);
        vkUnmapMemory(device, stagingMemory);

        // --- Create image ---
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(device, &imageInfo, nullptr, &fontImage_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create font image!" << std::endl;
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingMemory, nullptr);
            return;
        }

        VkMemoryRequirements imgMemReq;
        vkGetImageMemoryRequirements(device, fontImage_, &imgMemReq);

        VkMemoryAllocateInfo imgAllocInfo{};
        imgAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        imgAllocInfo.allocationSize = imgMemReq.size;
        imgAllocInfo.memoryTypeIndex = context_->findMemoryType(imgMemReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &imgAllocInfo, nullptr, &fontMemory_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to allocate font image memory!" << std::endl;
            vkDestroyImage(device, fontImage_, nullptr);
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingMemory, nullptr);
            return;
        }

        vkBindImageMemory(device, fontImage_, fontMemory_, 0);

        // --- Create temporary command pool for transfer ---
        VkCommandPool tempCommandPool;
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        cmdPoolInfo.queueFamilyIndex = context_->getGraphicsQueueFamily();

        if (vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &tempCommandPool) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create temp command pool!" << std::endl;
            vkDestroyImage(device, fontImage_, nullptr);
            vkFreeMemory(device, fontMemory_, nullptr);
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingMemory, nullptr);
            return;
        }

        // --- Allocate command buffer ---
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandPool = tempCommandPool;
        cmdAllocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        // Transition to TRANSFER_DST
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = fontImage_;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

        vkCmdCopyBufferToImage(cmd, stagingBuffer, fontImage_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Transition to SHADER_READ_ONLY
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkEndCommandBuffer(cmd);

        // Submit and wait
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        vkQueueSubmit(context_->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(context_->getGraphicsQueue());

        // Cleanup temp command pool (this also frees the command buffer)
        vkDestroyCommandPool(device, tempCommandPool, nullptr);

        // Cleanup staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        // --- Create image view ---
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = fontImage_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &fontView_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create font image view!" << std::endl;
            return;
        }

        // --- Create sampler ---
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &fontSampler_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create font sampler!" << std::endl;
            return;
        }

        // --- Create descriptor pool ---
        VkDescriptorPoolSize descPoolSize{};
        descPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descPoolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo descPoolInfo{};
        descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descPoolInfo.poolSizeCount = 1;
        descPoolInfo.pPoolSizes = &descPoolSize;
        descPoolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device, &descPoolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create descriptor pool!" << std::endl;
            return;
        }

        // --- Allocate descriptor set ---
        VkDescriptorSetAllocateInfo descSetAllocInfo{};
        descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descSetAllocInfo.descriptorPool = descriptorPool_;
        descSetAllocInfo.descriptorSetCount = 1;
        descSetAllocInfo.pSetLayouts = &descriptorSetLayout_;

        if (vkAllocateDescriptorSets(device, &descSetAllocInfo, &descriptorSet_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to allocate descriptor set!" << std::endl;
            return;
        }

        // --- Update descriptor set ---
        VkDescriptorImageInfo descImageInfo{};
        descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descImageInfo.imageView = fontView_;
        descImageInfo.sampler = fontSampler_;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet_;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &descImageInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

        std::cout << "[OK] UI font texture created (" << width << "x" << height << ")" << std::endl;
    }

} // namespace libre::ui