// LibreUI/include/LibreUI/Widgets/Panel.h
// Collapsible panel container widget

#pragma once

#include <LibreUI/Widget.h>
#include <LibreUI/Theme.h>
#include <LibreUI/Export.h>

namespace LibreUI {

    class LIBREUI_API Panel : public Widget {
    public:
        Panel(const std::string& title = "");

        void layout(const Rect& available) override;
        void draw(Renderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        // Public properties
        std::string title;
        bool collapsible = true;
        bool collapsed = false;

    private:
        Rect headerBounds_;
        Rect contentBounds_;
    };

} // namespace LibreUI