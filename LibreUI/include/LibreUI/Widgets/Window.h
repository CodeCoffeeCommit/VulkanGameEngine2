// LibreUI/include/LibreUI/Widgets/Window.h
// Floating window widget with title bar

#pragma once

#include <LibreUI/Widget.h>
#include <LibreUI/Theme.h>

namespace LibreUI {

    class Window : public Widget {
    public:
        Window(const std::string& title = "");

        void layout(const Rect& available) override;
        void draw(Renderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        // Public properties
        std::string title;
        bool isOpen = true;
        bool closable = true;
        bool draggable = true;
        bool resizable = false;  // Future feature

        // Callbacks
        ClickCallback onClose;

    private:
        Rect titleBarBounds_;
        Rect closeButtonBounds_;
        Rect contentBounds_;

        bool dragging_ = false;
        float dragOffsetX_ = 0;
        float dragOffsetY_ = 0;
        bool closeHovered_ = false;
    };

} // namespace LibreUI