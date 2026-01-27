// LibreUI/include/LibreUI/Widgets/Dropdown.h
// Dropdown selector widget

#pragma once

#include <LibreUI/Widget.h>
#include <LibreUI/Theme.h>
#include <vector>

namespace LibreUI {

    class Dropdown : public Widget {
    public:
        Dropdown();

        void draw(Renderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        // Public properties
        std::vector<std::string> items;
        int selectedIndex = 0;
        IndexCallback onSelect;

        bool open = false;
        int hoveredItem = -1;

    private:
        Rect getDropdownBounds() const;
    };

} // namespace LibreUI