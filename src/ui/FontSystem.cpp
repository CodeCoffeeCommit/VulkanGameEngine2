// src/ui/FontSystem.cpp
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

        // Cleanup atlas
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

        // Cleanup FreeType faces
        for (auto& [name, family] : families_) {
            for (auto& [weight, face] : family.faces) {
                FT_Done_Face(face);
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

    bool FontSystem::loadFont(const std::string& name, const std::string& path,
        FontWeight weight) {
        // Create or get family
        FontFamily& family = families_[name];
        family.name = name;

        int weightKey = static_cast<int>(weight);
        family.weightPaths[weightKey] = path;

        // Load the FreeType face
        FT_Face face;
        FT_Error err = FT_New_Face(ftLibrary_, path.c_str(), 0, &face);
        if (err) {
            std::cerr << "[FontSystem] Failed to load font: " << path
                << " (error: " << err << ")\n";
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
        // Build cache key
        std::string key = name + "_" + std::to_string(size) + "_" +
            std::to_string(static_cast<int>(weight));

        // Check cache
        auto it = fontCache_.find(key);
        if (it != fontCache_.end()) {
            return it->second.get();
        }

        // Find the family and face
        auto famIt = families_.find(name);
        if (famIt == families_.end()) {
            std::cerr << "[FontSystem] Font family not found: " << name << "\n";
            return nullptr;
        }

        int weightKey = static_cast<int>(weight);
        auto faceIt = famIt->second.faces.find(weightKey);
        if (faceIt == famIt->second.faces.end()) {
            // Fallback to regular weight
            faceIt = famIt->second.faces.find(static_cast<int>(FontWeight::Regular));
            if (faceIt == famIt->second.faces.end()) {
                std::cerr << "[FontSystem] No face found for: " << name << "\n";
                return nullptr;
            }
        }

        FT_Face ftFace = faceIt->second;

        // Set pixel size
        FT_Set_Pixel_Sizes(ftFace, 0, size);

        // Create FontFace
        auto fontFace = std::make_unique<FontFace>();
        fontFace->name = name;
        fontFace->size = size;
        fontFace->lineHeight = ftFace->size->metrics.height / 64.0f;
        fontFace->ascender = ftFace->size->metrics.ascender / 64.0f;
        fontFace->descender = ftFace->size->metrics.descender / 64.0f;

        // Pre-rasterize common ASCII glyphs (32-126)
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
            return false;  // Glyph not found
        }

        FT_Error err = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
        if (err) return false;

        err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (err) return false;

        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap& bitmap = slot->bitmap;

        outGlyph.size = glm::ivec2(bitmap.width, bitmap.rows);
        outGlyph.bearing = glm::ivec2(slot->bitmap_left, slot->bitmap_top);
        outGlyph.advance = slot->advance.x;  // In 1/64 pixels

        // Pack into atlas
        if (bitmap.width > 0 && bitmap.rows > 0) {
            if (!packGlyphIntoAtlas(bitmap.buffer, bitmap.width, bitmap.rows,
                outGlyph.uvMin, outGlyph.uvMax)) {
                return false;
            }
        }
        else {
            // Whitespace character - no texture needed
            outGlyph.uvMin = outGlyph.uvMax = glm::vec2(0);
        }

        return true;
    }

    bool FontSystem::packGlyphIntoAtlas(const uint8_t* bitmap, int width, int height,
        glm::vec2& uvMin, glm::vec2& uvMax) {
        int paddedW = width + GLYPH_PADDING * 2;
        int paddedH = height + GLYPH_PADDING * 2;

        // Check if we need to start a new row
        if (atlasPage_.currentX + paddedW > atlasPage_.width) {
            atlasPage_.currentX = 0;
            atlasPage_.currentY += atlasPage_.rowHeight + GLYPH_PADDING;
            atlasPage_.rowHeight = 0;
        }

        // Check if we need to grow the atlas
        if (atlasPage_.currentY + paddedH > atlasPage_.height) {
            if (atlasPage_.height < MAX_ATLAS_SIZE) {
                growAtlas();
                // Retry after growing
                return packGlyphIntoAtlas(bitmap, width, height, uvMin, uvMax);
            }
            else {
                std::cerr << "[FontSystem] Atlas is full!\n";
                return false;
            }
        }

        // Copy glyph bitmap to atlas (with padding)
        int destX = atlasPage_.currentX + GLYPH_PADDING;
        int destY = atlasPage_.currentY + GLYPH_PADDING;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int srcIdx = y * width + x;
                int dstIdx = (destY + y) * atlasPage_.width + (destX + x);
                atlasPage_.pixels[dstIdx] = bitmap[srcIdx];
            }
        }

        // Calculate UVs
        uvMin.x = static_cast<float>(destX) / atlasPage_.width;
        uvMin.y = static_cast<float>(destY) / atlasPage_.height;
        uvMax.x = static_cast<float>(destX + width) / atlasPage_.width;
        uvMax.y = static_cast<float>(destY + height) / atlasPage_.height;

        // Update cursor
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

        // Create Vulkan image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { (uint32_t)width, (uint32_t)height, 1 };
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

        // Allocate memory
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, atlasPage_.image, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = context_->findMemoryType(
            memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &atlasPage_.memory) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to allocate atlas memory\n";
            return false;
        }

        vkBindImageMemory(device, atlasPage_.image, atlasPage_.memory, 0);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = atlasPage_.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8_UNORM;
        viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        if (vkCreateImageView(device, &viewInfo, nullptr, &atlasPage_.view) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to create atlas image view\n";
            return false;
        }

        // Create sampler with linear filtering for smooth text
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
        int newSize = std::min(atlasPage_.height * 2, MAX_ATLAS_SIZE);
        std::cout << "[FontSystem] Growing atlas to " << newSize << "x" << newSize << "\n";

        VkDevice device = context_->getDevice();
        vkDeviceWaitIdle(device);

        // Cleanup old
        if (atlasPage_.view) vkDestroyImageView(device, atlasPage_.view, nullptr);
        if (atlasPage_.image) vkDestroyImage(device, atlasPage_.image, nullptr);
        if (atlasPage_.memory) vkFreeMemory(device, atlasPage_.memory, nullptr);

        // Recreate with new size
        createAtlas(newSize, newSize);

        // Clear cache to force re-rasterization
        fontCache_.clear();
    }

    void FontSystem::flushAtlas(VkCommandBuffer cmd) {
        if (!atlasPage_.dirty || !context_) return;
        uploadAtlasToGPU(cmd);
        atlasPage_.dirty = false;
    }

    void FontSystem::uploadAtlasToGPU(VkCommandBuffer cmd) {
        VkDevice device = context_->getDevice();

        // Create staging buffer
        VkDeviceSize size = atlasPage_.width * atlasPage_.height;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

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
        allocInfo.memoryTypeIndex = context_->findMemoryType(
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
            std::cerr << "[FontSystem] Failed to allocate staging memory\n";
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            return;
        }

        vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

        // Copy data to staging
        void* data;
        vkMapMemory(device, stagingMemory, 0, size, 0, &data);
        memcpy(data, atlasPage_.pixels.data(), size);
        vkUnmapMemory(device, stagingMemory);

        // Transition image to transfer dst
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = atlasPage_.image;
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
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
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { (uint32_t)atlasPage_.width, (uint32_t)atlasPage_.height, 1 };

        vkCmdCopyBufferToImage(cmd, stagingBuffer, atlasPage_.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Transition to shader read
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Cleanup staging resources
        vkQueueWaitIdle(context_->getGraphicsQueue());
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
    }

    glm::vec2 FontSystem::measureText(const std::string& text, FontFace* font) {
        if (!font) return glm::vec2(0);

        float width = 0;
        float maxWidth = 0;
        int lines = 1;

        for (char c : text) {
            if (c == '\n') {
                maxWidth = std::max(maxWidth, width);
                width = 0;
                lines++;
                continue;
            }

            auto it = font->glyphs.find(static_cast<uint32_t>(c));
            if (it != font->glyphs.end()) {
                width += it->second.advance / 64.0f;
            }
        }

        maxWidth = std::max(maxWidth, width);
        return glm::vec2(maxWidth, font->lineHeight * lines);
    }

    glm::vec2 FontSystem::measureText(const std::string& text, const std::string& fontName,
        int size, FontWeight weight) {
        FontFace* font = getFont(fontName, size, weight);
        return measureText(text, font);
    }

    std::vector<std::string> FontSystem::wrapText(const std::string& text, float maxWidth,
        FontFace* font) {
        std::vector<std::string> lines;
        if (!font || maxWidth <= 0) {
            lines.push_back(text);
            return lines;
        }

        std::istringstream stream(text);
        std::string word;
        std::string currentLine;
        float currentWidth = 0;

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

        return delta.x;  // In 1/64 pixels
    }

} // namespace libre::ui