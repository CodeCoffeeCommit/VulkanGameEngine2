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

        if (fontSampler_) vkDestroySampler(device, fontSampler_, nullptr);
        if (fontView_) vkDestroyImageView(device, fontView_, nullptr);
        if (fontImage_) vkDestroyImage(device, fontImage_, nullptr);
        if (fontMemory_) vkFreeMemory(device, fontMemory_, nullptr);

        if (vertexBuffer_) vkDestroyBuffer(device, vertexBuffer_, nullptr);
        if (vertexMemory_) vkFreeMemory(device, vertexMemory_, nullptr);

        if (descriptorPool_) vkDestroyDescriptorPool(device, descriptorPool_, nullptr);
        if (descriptorSetLayout_) vkDestroyDescriptorSetLayout(device, descriptorSetLayout_, nullptr);
        if (pipelineLayout_) vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        if (pipeline_) vkDestroyPipeline(device, pipeline_, nullptr);
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

            float u0 = atlasX / 16.0f;
            float v0 = atlasY / 6.0f;
            float u1 = (atlasX + 1) / 16.0f;
            float v1 = (atlasY + 1) / 6.0f;

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
        if (vertices_.empty()) return;
        if (pipeline_ == VK_NULL_HANDLE) {
            std::cerr << "[UIRenderer] ERROR: Pipeline is null!" << std::endl;
            return;
        }

        void* data;
        vkMapMemory(context_->getDevice(), vertexMemory_, 0,
            vertices_.size() * sizeof(UIVertex), 0, &data);
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

        // Dynamic states
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

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

        // Depth stencil - disabled for UI
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depthStencil.stencilTestEnable = VK_FALSE;

        // Color blending - alpha blending
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

        const int width = 128;
        const int height = 48;
        std::vector<unsigned char> pixels(width * height, 255);

        // Create image
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
            return;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, fontImage_, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = context_->findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &fontMemory_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to allocate font image memory!" << std::endl;
            return;
        }

        vkBindImageMemory(device, fontImage_, fontMemory_, 0);

        // Create image view
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

        // Create sampler
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

        // Create descriptor pool
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to create descriptor pool!" << std::endl;
            return;
        }

        // Allocate descriptor set
        VkDescriptorSetAllocateInfo descAllocInfo{};
        descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool = descriptorPool_;
        descAllocInfo.descriptorSetCount = 1;
        descAllocInfo.pSetLayouts = &descriptorSetLayout_;

        if (vkAllocateDescriptorSets(device, &descAllocInfo, &descriptorSet_) != VK_SUCCESS) {
            std::cerr << "[UIRenderer] Failed to allocate descriptor set!" << std::endl;
            return;
        }

        // Update descriptor set
        VkDescriptorImageInfo imageDescInfo{};
        imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageDescInfo.imageView = fontView_;
        imageDescInfo.sampler = fontSampler_;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet_;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageDescInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

        std::cout << "[OK] UI font texture created" << std::endl;
    }

} // namespace libre::ui