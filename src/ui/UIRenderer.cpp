// src/ui/UIRenderer.cpp
#include "UIRenderer.h"
#include "../render/VulkanContext.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <array>
#include <algorithm>

namespace libre::ui {

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        size_t fileSize = static_cast<size_t>(file.tellg());
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

        FontSystem& fonts = FontSystem::instance();
        if (fonts.init(context)) {
            bool fontsLoaded = fonts.loadFontFamily(FontSystem::DEFAULT_FONT,
                "fonts/Inter-Regular.ttf",
                "fonts/Inter-Light.ttf"
                "fonts/Inter-Bold.ttf",
                "fonts/Inter-Medium.ttf"
                
            );

            if (!fontsLoaded) {
                std::cerr << "[UIRenderer] Warning: Could not load Inter font family\n";
            }

            if (!fonts.loadFont(FontSystem::MONOSPACE_FONT, "fonts/JetBrainsMono-Regular.ttf")) {
                std::cerr << "[UIRenderer] Warning: Could not load monospace font\n";
            }

            fontSystemInitialized_ = true;
            createDescriptorResources();
            std::cout << "[OK] UIRenderer initialized with FreeType fonts\n";
        }
        else {
            std::cerr << "[UIRenderer] Warning: FontSystem failed to initialize\n";
            fontSystemInitialized_ = false;
        }
    }

    void UIRenderer::cleanup() {
        if (!context_) return;
        VkDevice device = context_->getDevice();

        vkDeviceWaitIdle(device);

        if (fontSystemInitialized_) {
            FontSystem::instance().shutdown();
            fontSystemInitialized_ = false;
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
        std::cout << "[OK] UIRenderer cleaned up\n";
    }

    void UIRenderer::begin(float screenWidth, float screenHeight) {
        screenWidth_ = screenWidth;
        screenHeight_ = screenHeight;
        vertices_.clear();
        clipStack_.clear();
    }

    void UIRenderer::end(VkCommandBuffer cmd) {
        if (fontSystemInitialized_) {
            FontSystem::instance().flushAtlas(cmd);
        }

        if (!vertices_.empty()) {
            flushBatch(cmd);
        }
    }

    // ========================================================================
    // DRAWING PRIMITIVES
    // ========================================================================

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

    // ========================================================================
    // TEXT RENDERING
    // ========================================================================

    void UIRenderer::drawText(const std::string& text, float x, float y,
        const Color& color, float size) {
        drawTextEx(text, x, y, color, size, FontSystem::DEFAULT_FONT, FontWeight::Regular);
    }

    void UIRenderer::drawTextEx(const std::string& text, float x, float y,
        const Color& color, float size,
        const std::string& fontName, FontWeight weight) {
        if (!fontSystemInitialized_ || text.empty()) return;

        FontSystem& fonts = FontSystem::instance();
        FontFace* font = fonts.getFont(fontName, static_cast<int>(size), weight);

        if (!font) {
            std::cerr << "[UIRenderer] Font not found: " << fontName << "\n";
            return;
        }

        glm::vec4 c = color.toVec4();
        float penX = x;
        float baseline = y + font->ascender;

        for (size_t i = 0; i < text.length(); ++i) {
            char ch = text[i];

            if (ch == '\n') {
                penX = x;
                baseline += font->lineHeight;
                continue;
            }

            if (ch < 32) continue;

            auto it = font->glyphs.find(static_cast<uint32_t>(ch));
            if (it == font->glyphs.end()) continue;

            const Glyph& g = it->second;

            if (g.size.x == 0 || g.size.y == 0) {
                penX += g.advance / 64.0f;
                continue;
            }

            float glyphX = penX + static_cast<float>(g.bearing.x);
            float glyphY = baseline - static_cast<float>(g.bearing.y);
            float glyphW = static_cast<float>(g.size.x);
            float glyphH = static_cast<float>(g.size.y);

            float x0 = glyphX, y0 = glyphY;
            float x1 = glyphX + glyphW, y1 = glyphY + glyphH;
            float u0 = g.uvMin.x, v0 = g.uvMin.y;
            float u1 = g.uvMax.x, v1 = g.uvMax.y;

            if (!clipStack_.empty()) {
                const Rect& clip = clipStack_.back();

                if (x0 < clip.x) {
                    float ratio = (clip.x - x0) / (x1 - x0);
                    u0 += ratio * (u1 - u0);
                    x0 = clip.x;
                }
                if (y0 < clip.y) {
                    float ratio = (clip.y - y0) / (y1 - y0);
                    v0 += ratio * (v1 - v0);
                    y0 = clip.y;
                }
                if (x1 > clip.right()) {
                    float ratio = (x1 - clip.right()) / (x1 - x0);
                    u1 -= ratio * (u1 - u0);
                    x1 = clip.right();
                }
                if (y1 > clip.bottom()) {
                    float ratio = (y1 - clip.bottom()) / (y1 - y0);
                    v1 -= ratio * (v1 - v0);
                    y1 = clip.bottom();
                }

                if (x1 <= x0 || y1 <= y0) {
                    penX += g.advance / 64.0f;
                    continue;
                }
            }

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

            penX += g.advance / 64.0f;
        }
    }

    Vec2 UIRenderer::measureText(const std::string& text, float size) {
        return measureTextEx(text, size, FontSystem::DEFAULT_FONT, FontWeight::Regular);
    }

    Vec2 UIRenderer::measureTextEx(const std::string& text, float size,
        const std::string& fontName, FontWeight weight) {
        if (!fontSystemInitialized_) {
            return Vec2{ text.length() * size * 0.5f, size };
        }

        FontSystem& fonts = FontSystem::instance();
        FontFace* font = fonts.getFont(fontName, static_cast<int>(size), weight);

        if (!font) {
            return Vec2{ text.length() * size * 0.5f, size };
        }

        glm::vec2 bounds = fonts.measureText(text, font);
        return Vec2{ bounds.x, bounds.y };
    }

    // ========================================================================
    // CLIPPING
    // ========================================================================

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

    // ========================================================================
    // INTERNAL: Vulkan Resources
    // ========================================================================

    void UIRenderer::flushBatch(VkCommandBuffer cmd) {
        if (vertices_.empty()) return;

        if (pipeline_ == VK_NULL_HANDLE) {
            std::cerr << "[UIRenderer] ERROR: Pipeline is null!\n";
            return;
        }

        if (vertexBuffer_ == VK_NULL_HANDLE || vertexMemory_ == VK_NULL_HANDLE) {
            std::cerr << "[UIRenderer] ERROR: Vertex buffer not initialized!\n";
            return;
        }

        void* data;
        VkResult mapResult = vkMapMemory(context_->getDevice(), vertexMemory_, 0,
            vertices_.size() * sizeof(UIVertex), 0, &data);
        if (mapResult != VK_SUCCESS) {
            std::cerr << "[UIRenderer] ERROR: Failed to map vertex memory!\n";
            return;
        }
        memcpy(data, vertices_.data(), vertices_.size() * sizeof(UIVertex));
        vkUnmapMemory(context_->getDevice(), vertexMemory_);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout_, 0, 1, &descriptorSet_, 0, nullptr);

        VkBuffer buffers[] = { vertexBuffer_ };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdDraw(cmd, static_cast<uint32_t>(vertices_.size()), 1, 0, 0);
    }

    void UIRenderer::createDescriptorResources() {
        VkDevice device = context_->getDevice();

        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create descriptor pool!\n";
            return;
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool_;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout_;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to allocate descriptor set!\n";
            return;
        }

        updateFontDescriptor();
    }

    void UIRenderer::updateFontDescriptor() {
        if (!fontSystemInitialized_) return;

        FontSystem& fonts = FontSystem::instance();

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = fonts.getAtlasView();
        imageInfo.sampler = fonts.getAtlasSampler();

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet_;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(context_->getDevice(), 1, &write, 0, nullptr);
    }

    void UIRenderer::createPipeline(VkRenderPass renderPass) {
        VkDevice device = context_->getDevice();

        std::vector<char> vertShaderCode;
        std::vector<char> fragShaderCode;

        try {
            vertShaderCode = readFile("shaders/compiled/ui.vert.spv");
            fragShaderCode = readFile("shaders/compiled/ui.frag.spv");
        }
        catch (const std::exception& e) {
            std::cerr << "[UIRenderer] Failed to load shaders: " << e.what() << "\n";
            return;
        }

        VkShaderModuleCreateInfo vertCreateInfo{};
        vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertCreateInfo.codeSize = vertShaderCode.size();
        vertCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());

        VkShaderModule vertShaderModule;
        if (vkCreateShaderModule(device, &vertCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create vertex shader module!\n";
            return;
        }

        VkShaderModuleCreateInfo fragCreateInfo{};
        fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragCreateInfo.codeSize = fragShaderCode.size();
        fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());

        VkShaderModule fragShaderModule;
        if (vkCreateShaderModule(device, &fragCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create fragment shader module!\n";
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            return;
        }

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

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

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

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Descriptor set layout for font texture
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
            std::cerr << "[UIRenderer] Failed to create descriptor set layout!\n";
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            return;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create pipeline layout!\n";
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            return;
        }

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

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

        if (result != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create graphics pipeline! Error: " << result << "\n";
            return;
        }

        std::cout << "[OK] UI pipeline created\n";
    }

    void UIRenderer::createBuffers() {
        VkDevice device = context_->getDevice();

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MAX_VERTICES * sizeof(UIVertex);
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create vertex buffer!\n";
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
            std::cerr << "[UIRenderer] Failed to allocate vertex buffer memory!\n";
            return;
        }

        if (vkBindBufferMemory(device, vertexBuffer_, vertexMemory_, 0) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to bind vertex buffer memory!\n";
            return;
        }

        std::cout << "[OK] UI vertex buffer created\n";
    }

} // namespace libre::ui