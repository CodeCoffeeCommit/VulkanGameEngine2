// LibreUI/include/LibreUI/Widgets/Label.h
// Simple text label widget

#pragma once

#include <LibreUI/Widget.h>
#include <LibreUI/Theme.h>

#include <LibreUI/Export.h>

namespace LibreUI {

    class LIBREUI_API Label : public Widget {
    public:
        Label(const std::string& text = "");

        void draw(Renderer& renderer) override;

        // Public properties
        std::string text;
        Color color;
        float fontSize = 13.0f;
    };

} // namespace LibreUI