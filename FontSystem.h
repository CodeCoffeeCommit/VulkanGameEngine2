// src/ui/FontSystem.h
#pragma once

#include <vulkan/vulkan.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class VulkanContext;

namespace libre::ui {

    // Individual glyph metrics and atlas location
    struct Glyph {
        glm::vec2 uvMin;      // Top-left UV in atlas
        glm::vec2 uvMax;      // Bottom-right UV in atlas
        glm::ivec2 size;      // Glyph size in pixels
        glm::ivec2 bearing;   // Offset from baseline to top-left
        int advance;          // Horizontal advance (in 1/64 pixels)
    };

    // Font face at a specific size
    struct FontFace {
        std::string name;
        int size;
        float lineHeight;
        float ascender;
        float descender;
        std::unordered_map<uint32_t, Glyph> glyphs;  // Codepoint -> Glyph
    };

    // Atlas page for storing glyphs
    struct AtlasPage {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        int width = 0;
        int height = 0;
        int currentX = 0;   // Packing cursor
        int currentY = 0;
        int rowHeight = 0;  // Tallest glyph in current row
        std::vector<uint8_t> pixels;  // CPU-side for updates
        bool dirty = false;
    };

    // Font weight/style variants
    enum class FontWeight { Regular, Bold, Light };
    enum class FontStyle { Normal, Italic };

    // Font family with multiple weights/styles
    struct FontFamily {
        std::string name;
        std::unordered_map<int, std::string> weightPaths;  // weight -> file path
        std::unordered_map<int, FT_Face> faces;            // weight -> loaded face
    };

    class FontSystem {
    public:
        static FontSystem& instance();

        bool init(VulkanContext* context);
        void shutdown();

        // Font loading
        bool loadFont(const std::string& name, const std::string& path,
            FontWeight weight = FontWeight::Regular);
        bool loadFontFamily(const std::string& familyName,
            const std::string& regularPath,
            const std::string& boldPath = "",
            const std::string& lightPath = "");

        // Get/create font at specific size
        FontFace* getFont(const std::string& name, int size,
            FontWeight weight = FontWeight::Regular);

        // Text measurement
        glm::vec2 measureText(const std::string& text, FontFace* font);
        glm::vec2 measureText(const std::string& text, const std::string& fontName,
            int size, FontWeight weight = FontWeight::Regular);

        // Word wrapping helper
        std::vector<std::string> wrapText(const std::string& text, float maxWidth,
            FontFace* font);

        // Atlas access for rendering
        VkImageView getAtlasView() const { return atlasPage_.view; }
        VkSampler getAtlasSampler() const { return sampler_; }
        VkDescriptorSet getDescriptorSet() const { return descriptorSet_; }

        // Update atlas to GPU if dirty
        void flushAtlas(VkCommandBuffer cmd);

        // Default fonts (like Blender's Inter/DejaVu)
        static constexpr const char* DEFAULT_FONT = "default";
        static constexpr const char* MONOSPACE_FONT = "mono";
        static constexpr const char* ICON_FONT = "icons";

    private:
        FontSystem() = default;
        ~FontSystem() = default;

        bool initFreeType();
        bool createAtlas(int width, int height);
        bool rasterizeGlyph(FT_Face face, uint32_t codepoint, Glyph& outGlyph);
        bool packGlyphIntoAtlas(const uint8_t* bitmap, int width, int height,
            glm::vec2& uvMin, glm::vec2& uvMax);
        void growAtlas();
        void uploadAtlasToGPU(VkCommandBuffer cmd);

        // Kerning support
        int getKerning(FT_Face face, uint32_t left, uint32_t right);

        VulkanContext* context_ = nullptr;
        FT_Library ftLibrary_ = nullptr;

        // Loaded font families
        std::unordered_map<std::string, FontFamily> families_;

        // Cached font faces at specific sizes
        // Key: "fontname_size_weight"
        std::unordered_map<std::string, std::unique_ptr<FontFace>> fontCache_;

        // Glyph atlas
        AtlasPage atlasPage_;
        VkSampler sampler_ = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

        // Default atlas size (grows as needed)
        static constexpr int INITIAL_ATLAS_SIZE = 1024;
        static constexpr int MAX_ATLAS_SIZE = 4096;
        static constexpr int GLYPH_PADDING = 2;  // Prevent bleeding
    };

    // ============================================================================
    // TEXT LAYOUT ENGINE
    // ============================================================================

    struct TextRun {
        std::string text;
        FontFace* font;
        glm::vec4 color;
        float startX;
    };

    struct LayoutGlyph {
        Glyph* glyph;
        glm::vec2 position;
        glm::vec4 color;
    };

    enum class TextAlign { Left, Center, Right };
    enum class TextVAlign { Top, Middle, Bottom };

    struct TextLayoutOptions {
        float maxWidth = 0;           // 0 = no wrap
        float lineSpacing = 1.2f;     // Multiplier on line height
        TextAlign align = TextAlign::Left;
        TextVAlign valign = TextVAlign::Top;
        bool ellipsis = false;        // Truncate with "..." if too long
        int maxLines = 0;             // 0 = unlimited
    };

    class TextLayout {
    public:
        void layout(const std::string& text, FontFace* font,
            const glm::vec4& color, const TextLayoutOptions& options);

        // Rich text support: [b]bold[/b], [color=#ff0000]red[/color]
        void layoutRich(const std::string& richText, FontFace* defaultFont,
            const glm::vec4& defaultColor, const TextLayoutOptions& options);

        const std::vector<LayoutGlyph>& getGlyphs() const { return glyphs_; }
        glm::vec2 getBounds() const { return bounds_; }
        int getLineCount() const { return lineCount_; }

    private:
        std::vector<LayoutGlyph> glyphs_;
        glm::vec2 bounds_{ 0, 0 };
        int lineCount_ = 0;
    };

} // namespace libre::ui