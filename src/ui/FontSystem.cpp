// src/ui/FontSystem.cpp
// SIMPLIFIED VERSION - Avoids MSVC Internal Compiler Error

#include "FontSystem.h"
#include "../render/VulkanContext.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <sstream>

namespace libre::ui {

    FontSystem& FontSystem::instance() {
        static FontSystem inst;
        return inst;
    }

    bool FontSystem::init(VulkanContext* context) {
        context_ = context;

        if (!initFreeType()) {
            std::cerr << "[FontSystem] Failed to initialize FreeType\n";
            return false;
        }

        if (!createAtlas(INITIAL_ATLAS_SIZE, INITIAL_ATLAS_SIZE)) {
            std::cerr << "[FontSystem] Failed to create glyph atlas\n";
            return false;
        }

        std::cout << "[OK] FontSystem initialized\n";
        return true;
    }

    bool FontSystem::initFreeType() {
        FT_Error err = FT_Init_FreeType(&ftLibrary_);
        if (err) {
            std::cerr << "[FontSystem] FT_Init_FreeType failed: " << err << "\n";
            return false;
        }
        return true;
    }

    void FontSystem::shutdown() {
        if (!context_) return;

        VkDevice device = context_->getDevice();
        vkDeviceWaitIdle(device);

        if (sampler_) {
            vkDestroySampler(device, sampler_, nullptr);
            sampler_ = VK_NULL_HANDLE;
        }
        if (atlasPage_.view) {
            vkDestroyImageView(device, atlasPage_.view, nullptr);
            atlasPage_.view = VK_NULL_HANDLE;
        }
        if (atlasPage_.image) {
            vkDestroyImage(device, atlasPage_.image, nullptr);
            atlasPage_.image = VK_NULL_HANDLE;
        }
        if (atlasPage_.memory) {
            vkFreeMemory(device, atlasPage_.memory, nullptr);
            atlasPage_.memory = VK_NULL_HANDLE;
        }

        // Cleanup FreeType faces - simple iterator style
        std::unordered_map<std::string, FontFamily>::iterator famIt;
        for (famIt = families_.begin(); famIt != families_.end(); ++famIt) {
            std::unordered_map<int, FT_Face>::iterator faceIt;
            for (faceIt = famIt->second.faces.begin(); faceIt != famIt->second.faces.end(); ++faceIt) {
                FT_Done_Face(faceIt->second);
            }
        }
        families_.clear();
        fontCache_.clear();

        if (ftLibrary_) {
            FT_Done_FreeType(ftLibrary_);
            ftLibrary_ = nullptr;
        }

        context_ = nullptr;
        std::cout << "[OK] FontSystem shutdown\n";
    }

    bool FontSystem::loadFont(const std::string& name, const std::string& path, FontWeight weight) {
        FontFamily& family = families_[name];
        family.name = name;

        int weightKey = static_cast<int>(weight);
        family.weightPaths[weightKey] = path;

        FT_Face face;
        FT_Error err = FT_New_Face(ftLibrary_, path.c_str(), 0, &face);
        if (err) {
            std::cerr << "[FontSystem] Failed to load font: " << path << " (error: " << err << ")\n";
            return false;
        }

        family.faces[weightKey] = face;
        std::cout << "[OK] Loaded font: " << name << " (" << path << ")\n";
        return true;
    }

    bool FontSystem::loadFontFamily(const std::string& familyName,
        const std::string& regularPath,
        const std::string& boldPath,
        const std::string& lightPath) {
        bool ok = loadFont(familyName, regularPath, FontWeight::Regular);
        if (!boldPath.empty()) {
            loadFont(familyName, boldPath, FontWeight::Bold);
        }
        if (!lightPath.empty()) {
            loadFont(familyName, lightPath, FontWeight::Light);
        }
        return ok;
    }

    FontFace* FontSystem::getFont(const std::string& name, int size, FontWeight weight) {
        std::string key = name + "_" + std::to_string(size) + "_" + std::to_string(static_cast<int>(weight));

        std::unordered_map<std::string, std::unique_ptr<FontFace>>::iterator cacheIt = fontCache_.find(key);
        if (cacheIt != fontCache_.end()) {
            return cacheIt->second.get();
        }

        std::unordered_map<std::string, FontFamily>::iterator famIt = families_.find(name);
        if (famIt == families_.end()) {
            std::cerr << "[FontSystem] Font family not found: " << name << "\n";
            return nullptr;
        }

        int weightKey = static_cast<int>(weight);
        std::unordered_map<int, FT_Face>::iterator faceIt = famIt->second.faces.find(weightKey);
        if (faceIt == famIt->second.faces.end()) {
            faceIt = famIt->second.faces.find(static_cast<int>(FontWeight::Regular));
            if (faceIt == famIt->second.faces.end()) {
                std::cerr << "[FontSystem] No face found for: " << name << "\n";
                return nullptr;
            }
        }

        FT_Face ftFace = faceIt->second;
        FT_Set_Pixel_Sizes(ftFace, 0, size);

        std::unique_ptr<FontFace> fontFace(new FontFace());
        fontFace->name = name;
        fontFace->size = size;
        fontFace->lineHeight = ftFace->size->metrics.height / 64.0f;
        fontFace->ascender = ftFace->size->metrics.ascender / 64.0f;
        fontFace->descender = ftFace->size->metrics.descender / 64.0f;

        for (uint32_t cp = 32; cp <= 126; ++cp) {
            Glyph glyph;
            if (rasterizeGlyph(ftFace, cp, glyph)) {
                fontFace->glyphs[cp] = glyph;
            }
        }

        FontFace* ptr = fontFace.get();
        fontCache_[key] = std::move(fontFace);
        return ptr;
    }

    bool FontSystem::rasterizeGlyph(FT_Face face, uint32_t codepoint, Glyph& outGlyph) {
        FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);
        if (glyphIndex == 0 && codepoint != 0) {
            return false;
        }

        FT_Error err = FT_Load_Glyph(face, glyphIndex, FT_LOAD_TARGET_LIGHT);
        if (err) return false;

        err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LIGHT);
        if (err) return false;

        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap& bitmap = slot->bitmap;

        outGlyph.size = glm::ivec2(bitmap.width, bitmap.rows);
        outGlyph.bearing = glm::ivec2(slot->bitmap_left, slot->bitmap_top);
        outGlyph.advance = slot->advance.x;

        if (bitmap.width > 0 && bitmap.rows > 0) {
            if (!packGlyphIntoAtlas(bitmap.buffer, bitmap.width, bitmap.rows,
                outGlyph.uvMin, outGlyph.uvMax)) {
                return false;
            }
        }
        else {
            outGlyph.uvMin = outGlyph.uvMax = glm::vec2(0);
        }

        return true;
    }

    bool FontSystem::packGlyphIntoAtlas(const uint8_t* bitmap, int width, int height,
        glm::vec2& uvMin, glm::vec2& uvMax) {
        int paddedW = width + GLYPH_PADDING * 2;
        int paddedH = height + GLYPH_PADDING * 2;

        if (atlasPage_.currentX + paddedW > atlasPage_.width) {
            atlasPage_.currentX = 0;
            atlasPage_.currentY += atlasPage_.rowHeight + GLYPH_PADDING;
            atlasPage_.rowHeight = 0;
        }

        if (atlasPage_.currentY + paddedH > atlasPage_.height) {
            if (atlasPage_.height < MAX_ATLAS_SIZE) {
                growAtlas();
                return packGlyphIntoAtlas(bitmap, width, height, uvMin, uvMax);
            }
            else {
                std::cerr << "[FontSystem] Atlas is full!\n";
                return false;
            }
        }

        int destX = atlasPage_.currentX + GLYPH_PADDING;
        int destY = atlasPage_.currentY + GLYPH_PADDING;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int srcIdx = y * width + x;
                int dstIdx = (destY + y) * atlasPage_.width + (destX + x);
                atlasPage_.pixels[dstIdx] = bitmap[srcIdx];
            }
        }

        uvMin.x = static_cast<float>(destX) / atlasPage_.width;
        uvMin.y = static_cast<float>(destY) / atlasPage_.height;
        uvMax.x = static_cast<float>(destX + width) / atlasPage_.width;
        uvMax.y = static_cast<float>(destY + height) / atlasPage_.height;

        atlasPage_.currentX += paddedW;
        atlasPage_.rowHeight = std::max(atlasPage_.rowHeight, paddedH);
        atlasPage_.dirty = true;

        return true;
    }

    bool FontSystem::createAtlas(int width, int height) {
        VkDevice device = context_->getDevice();

        atlasPage_.width = width;
        atlasPage_.height = height;
        atlasPage_.pixels.resize(width * height, 0);
        atlasPage_.currentX = 0;
        atlasPage_.currentY = 0;
        atlasPage_.rowHeight = 0;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(device, &imageInfo, nullptr, &atlasPage_.image) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to create atlas image\n";
            return false;
        }

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, atlasPage_.image, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = context_->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &atlasPage_.memory) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to allocate atlas memory\n";
            return false;
        }

        vkBindImageMemory(device, atlasPage_.image, atlasPage_.memory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = atlasPage_.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &atlasPage_.view) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to create atlas image view\n";
            return false;
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to create atlas sampler\n";
            return false;
        }

        atlasPage_.dirty = true;
        std::cout << "[OK] FontSystem atlas created (" << width << "x" << height << ")\n";
        return true;
    }

    void FontSystem::growAtlas() {
        int newSize = atlasPage_.height * 2;
        if (newSize > MAX_ATLAS_SIZE) newSize = MAX_ATLAS_SIZE;

        std::cout << "[FontSystem] Growing atlas to " << newSize << "x" << newSize << "\n";

        VkDevice device = context_->getDevice();
        vkDeviceWaitIdle(device);

        if (atlasPage_.view) vkDestroyImageView(device, atlasPage_.view, nullptr);
        if (atlasPage_.image) vkDestroyImage(device, atlasPage_.image, nullptr);
        if (atlasPage_.memory) vkFreeMemory(device, atlasPage_.memory, nullptr);

        createAtlas(newSize, newSize);
        fontCache_.clear();
    }

    void FontSystem::flushAtlas(VkCommandBuffer cmd) {
        if (!atlasPage_.dirty || !context_) return;
        uploadAtlasToGPU(cmd);
        atlasPage_.dirty = false;
    }

    // FIXED: Uses separate command buffer (outside render pass)
    void FontSystem::uploadAtlasToGPU(VkCommandBuffer) {
        VkDevice device = context_->getDevice();
        VkQueue graphicsQueue = context_->getGraphicsQueue();
        VkCommandPool commandPool = context_->getCommandPool();

        VkDeviceSize size = static_cast<VkDeviceSize>(atlasPage_.width) * atlasPage_.height;

        // Create staging buffer
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = size;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to create staging buffer\n";
            return;
        }

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = context_->findMemoryType(memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to allocate staging memory\n";
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            return;
        }

        vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

        // Copy pixels to staging buffer
        void* data = nullptr;
        vkMapMemory(device, stagingMemory, 0, size, 0, &data);
        memcpy(data, atlasPage_.pixels.data(), static_cast<size_t>(size));
        vkUnmapMemory(device, stagingMemory);

        // Create transfer command buffer
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandPool = commandPool;
        cmdAllocInfo.commandBufferCount = 1;

        VkCommandBuffer transferCmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(device, &cmdAllocInfo, &transferCmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(transferCmd, &beginInfo);

        // Transition to transfer dst
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = atlasPage_.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(transferCmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
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
        region.imageOffset.x = 0;
        region.imageOffset.y = 0;
        region.imageOffset.z = 0;
        region.imageExtent.width = static_cast<uint32_t>(atlasPage_.width);
        region.imageExtent.height = static_cast<uint32_t>(atlasPage_.height);
        region.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(transferCmd, stagingBuffer, atlasPage_.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Transition to shader read
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(transferCmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkEndCommandBuffer(transferCmd);

        // Submit and wait
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &transferCmd;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        // Cleanup
        vkFreeCommandBuffers(device, commandPool, 1, &transferCmd);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        std::cout << "[FontSystem] Atlas uploaded to GPU\n";
    }

    glm::vec2 FontSystem::measureText(const std::string& text, FontFace* font) {
        if (!font) return glm::vec2(0.0f);

        float width = 0.0f;
        float maxWidth = 0.0f;
        int lines = 1;

        for (size_t i = 0; i < text.size(); ++i) {
            char c = text[i];
            if (c == '\n') {
                if (width > maxWidth) maxWidth = width;
                width = 0.0f;
                lines++;
                continue;
            }

            uint32_t cp = static_cast<uint32_t>(static_cast<unsigned char>(c));
            std::unordered_map<uint32_t, Glyph>::iterator it = font->glyphs.find(cp);
            if (it != font->glyphs.end()) {
                width += it->second.advance / 64.0f;
            }
        }

        if (width > maxWidth) maxWidth = width;
        return glm::vec2(maxWidth, font->lineHeight * lines);
    }

    glm::vec2 FontSystem::measureText(const std::string& text, const std::string& fontName,
        int size, FontWeight weight) {
        FontFace* font = getFont(fontName, size, weight);
        return measureText(text, font);
    }

    std::vector<std::string> FontSystem::wrapText(const std::string& text, float maxWidth, FontFace* font) {
        std::vector<std::string> lines;
        if (!font || maxWidth <= 0) {
            lines.push_back(text);
            return lines;
        }

        std::istringstream stream(text);
        std::string word;
        std::string currentLine;
        float currentWidth = 0.0f;

        while (stream >> word) {
            float wordWidth = measureText(word, font).x;
            float spaceWidth = measureText(" ", font).x;

            if (currentWidth + wordWidth > maxWidth && !currentLine.empty()) {
                lines.push_back(currentLine);
                currentLine = word;
                currentWidth = wordWidth;
            }
            else {
                if (!currentLine.empty()) {
                    currentLine += " ";
                    currentWidth += spaceWidth;
                }
                currentLine += word;
                currentWidth += wordWidth;
            }
        }

        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }

        return lines;
    }

    int FontSystem::getKerning(FT_Face face, uint32_t left, uint32_t right) {
        if (!FT_HAS_KERNING(face)) return 0;

        FT_UInt leftIdx = FT_Get_Char_Index(face, left);
        FT_UInt rightIdx = FT_Get_Char_Index(face, right);

        FT_Vector delta;
        FT_Get_Kerning(face, leftIdx, rightIdx, FT_KERNING_DEFAULT, &delta);

        return static_cast<int>(delta.x);
    }

} // namespace libre::ui