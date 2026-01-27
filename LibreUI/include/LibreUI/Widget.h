// LibreUI/include/LibreUI/Widget.h
// Base class for all UI widgets

#pragma once

#include <LibreUI/Types.h>
#include <LibreUI/Events.h>
#include <vector>
#include <memory>
#include <string>

#include <LibreUI/Export.h>

namespace LibreUI {

    // Forward declaration - Renderer will be defined later
    class Renderer;

    // ============================================================================
    // BASE WIDGET
    // ============================================================================

    class LIBREUI_API Widget {
    public:
        virtual ~Widget() = default;

        // --------------------------------------------------------------------
        // Core virtual methods - override in derived classes
        // --------------------------------------------------------------------

        // Calculate layout given available space
        virtual void layout(const Rect& available);

        // Draw the widget using the renderer
        virtual void draw(Renderer& renderer);

        // Handle mouse input, return true if consumed
        virtual bool handleMouse(const MouseEvent& event);

        // Handle keyboard input, return true if consumed
        virtual bool handleKey(const KeyEvent& event);

        // --------------------------------------------------------------------
        // Child management
        // --------------------------------------------------------------------

        void addChild(std::unique_ptr<Widget> child);
        void removeChild(Widget* child);
        void clearChildren();

        const std::vector<std::unique_ptr<Widget>>& getChildren() const { return children_; }
        Widget* getParent() const { return parent_; }

        // --------------------------------------------------------------------
        // Public state
        // --------------------------------------------------------------------

        Rect bounds;
        bool visible = true;
        bool enabled = true;
        bool hovered = false;
        std::string id;  // Optional identifier for finding widgets

    protected:
        Widget* parent_ = nullptr;
        std::vector<std::unique_ptr<Widget>> children_;
    };

} // namespace LibreUI