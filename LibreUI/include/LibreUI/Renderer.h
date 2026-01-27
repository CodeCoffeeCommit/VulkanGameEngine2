// LibreUI/include/LibreUI/Renderer.h
// Abstract renderer interface - NO VULKAN IN THIS HEADER
// Implementations (VulkanRenderer) will include Vulkan headers

#pragma once

#include <LibreUI/Types.h>
#include <string>

#include <LibreUI/Export.h>

namespace LibreUI {

    // ============================================================================
    // FONT WEIGHT (for text rendering)
    // ============================================================================

    enum class FontWeight {
        Light,
        Regular,
        Medium,
        Bold
    };

    // ============================================================================
    // ABSTRACT RENDERER INTERFACE
    // ============================================================================

    class LIBREUI_API Renderer {
    public:
        virtual ~Renderer() = default;

        // --------------------------------------------------------------------
        // Frame lifecycle
        // --------------------------------------------------------------------

        virtual void begin(float screenWidth, float screenHeight) = 0;
        virtual void end() = 0;

        // --------------------------------------------------------------------
        // Primitive drawing
        // --------------------------------------------------------------------

        virtual void drawRect(const Rect& bounds, const Color& color) = 0;
        virtual void drawRoundedRect(const Rect& bounds, const Color& color, float radius) = 0;
        virtual void drawRectOutline(const Rect& bounds, const Color& color, float thickness = 1.0f) = 0;

        // --------------------------------------------------------------------
        // Text rendering
        // --------------------------------------------------------------------

        virtual void drawText(const std::string& text, float x, float y,
            const Color& color, float size = 13.0f) = 0;

        virtual void drawTextEx(const std::string& text, float x, float y,
            const Color& color, float size,
            const std::string& fontName,
            FontWeight weight = FontWeight::Regular) = 0;

        virtual Vec2 measureText(const std::string& text, float size = 13.0f) = 0;

        virtual Vec2 measureTextEx(const std::string& text, float size,
            const std::string& fontName,
            FontWeight weight = FontWeight::Regular) = 0;

        // --------------------------------------------------------------------
        // Clipping
        // --------------------------------------------------------------------

        virtual void pushClip(const Rect& bounds) = 0;
        virtual void popClip() = 0;

        // --------------------------------------------------------------------
        // State queries
        // --------------------------------------------------------------------

        virtual float getScreenWidth() const = 0;
        virtual float getScreenHeight() const = 0;
    };

} // namespace LibreUI