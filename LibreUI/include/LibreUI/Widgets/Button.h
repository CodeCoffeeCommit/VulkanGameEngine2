// LibreUI/include/LibreUI/Widgets/Button.h
// Clickable button widget

#pragma once

#include <LibreUI/Widget.h>
#include <LibreUI/Theme.h>
#include <LibreUI/Export.h>

namespace LibreUI {

    class LIBREUI_API Button : public Widget {
    public:
        Button(const std::string& text = "");

        void draw(Renderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        // Public properties
        std::string text;
        ClickCallback onClick;
        bool pressed = false;
    };

} // namespace LibreUI