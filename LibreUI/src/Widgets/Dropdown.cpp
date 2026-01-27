// LibreUI/src/Widgets/Dropdown.cpp
// Dropdown selector implementation

#include <LibreUI/Widgets/Dropdown.h>
#include <LibreUI/Renderer.h>

namespace LibreUI {

    Dropdown::Dropdown() {}

    void Dropdown::draw(Renderer& renderer) {
        auto& theme = GetTheme();

        // Main button area
        Color bgColor = hovered ? theme.buttonHover : theme.buttonBackground;
        renderer.drawRoundedRect(bounds, bgColor, theme.cornerRadius());

        // Selected text
        std::string displayText = items.empty() ? ""
            : (selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size()))
            ? items[selectedIndex] : "";

        float textY = bounds.y + (bounds.h - theme.fontSize()) / 2;
        renderer.drawText(displayText, bounds.x + theme.padding(), textY,
            theme.text, theme.fontSize());

        // Dropdown arrow
        float arrowX = bounds.right() - theme.padding() - 8;
        renderer.drawText("v", arrowX, textY, theme.textDim, theme.fontSize());

        // Draw dropdown list if open
        if (open && !items.empty()) {
            Rect dropBounds = getDropdownBounds();

            // Shadow
            renderer.drawRect({ dropBounds.x + 2, dropBounds.y + 2, dropBounds.w, dropBounds.h },
                { 0, 0, 0, 0.3f });

            // Background
            renderer.drawRect(dropBounds, theme.background);
            renderer.drawRectOutline(dropBounds, theme.border);

            // Items
            float y = dropBounds.y;
            for (int i = 0; i < static_cast<int>(items.size()); i++) {
                Rect itemBounds = { dropBounds.x, y, dropBounds.w, theme.dropdownItemHeight() };

                // Hover highlight
                if (i == hoveredItem) {
                    renderer.drawRect(itemBounds, theme.accent);
                }

                // Selected checkmark
                if (i == selectedIndex) {
                    renderer.drawText("*", itemBounds.x + 4,
                        itemBounds.y + (itemBounds.h - theme.fontSize()) / 2,
                        theme.accent, theme.fontSize());
                }

                // Item text
                renderer.drawText(items[i], itemBounds.x + 20,
                    itemBounds.y + (itemBounds.h - theme.fontSize()) / 2,
                    theme.text, theme.fontSize());

                y += theme.dropdownItemHeight();
            }
        }
    }

    bool Dropdown::handleMouse(const MouseEvent& event) {
        bool inside = bounds.contains(event.x, event.y);
        hovered = inside;

        if (open) {
            Rect dropBounds = getDropdownBounds();

            if (dropBounds.contains(event.x, event.y)) {
                // Find hovered item
                float y = dropBounds.y;
                hoveredItem = -1;

                for (int i = 0; i < static_cast<int>(items.size()); i++) {
                    Rect itemBounds = { dropBounds.x, y, dropBounds.w, GetTheme().dropdownItemHeight() };

                    if (itemBounds.contains(event.x, event.y)) {
                        hoveredItem = i;

                        if (event.pressed && event.button == MouseButton::Left) {
                            selectedIndex = i;
                            open = false;
                            if (onSelect) {
                                onSelect(i);
                            }
                            return true;
                        }
                    }

                    y += GetTheme().dropdownItemHeight();
                }

                return true;
            }
            else if (event.pressed) {
                // Click outside closes
                open = false;
                return inside;
            }
        }
        else {
            // Click to open
            if (inside && event.pressed && event.button == MouseButton::Left) {
                open = true;
                hoveredItem = -1;
                return true;
            }
        }

        return inside;
    }

    Rect Dropdown::getDropdownBounds() const {
        auto& theme = GetTheme();
        float height = items.size() * theme.dropdownItemHeight();
        return { bounds.x, bounds.bottom(), bounds.w, height };
    }

} // namespace LibreUI