// LibreUI/src/Widgets/Menu.cpp
// Menu bar and dropdown implementation

#include <LibreUI/Widgets/Menu.h>
#include <LibreUI/Renderer.h>
#include <algorithm>

namespace LibreUI {

    MenuBar::MenuBar() {}

    void MenuBar::layout(const Rect& available) {
        bounds = available;
        auto& theme = GetTheme();

        // Layout menu headers horizontally
        float x = bounds.x;
        for (auto& menu : menus_) {
            float textWidth = theme.padding() * 2 + menu.label.length() * theme.fontSize() * 0.6f;
            menu.bounds = { x, bounds.y, textWidth, bounds.h };
            x += menu.bounds.w;
        }
    }

    void MenuBar::draw(Renderer& renderer) {
        auto& theme = GetTheme();

        // Draw menu bar background
        renderer.drawRect(bounds, theme.backgroundDark);

        // Draw bottom border line
        renderer.drawRect({ bounds.x, bounds.bottom() - 1, bounds.w, 1 }, theme.border);

        // Draw menu headers
        for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
            auto& menu = menus_[i];
            bool isOpen = (openMenuIndex_ == i);
            bool isHovered = menu.hovered;

            // Header background
            if (isOpen) {
                renderer.drawRect(menu.bounds, theme.accent);
            }
            else if (isHovered) {
                renderer.drawRect(menu.bounds, theme.buttonHover);
            }

            // Header text
            float textX = menu.bounds.x + theme.padding();
            float textY = menu.bounds.y + (menu.bounds.h - theme.fontSize()) / 2;
            renderer.drawText(menu.label, textX, textY, theme.text, theme.fontSize());
        }

        // Draw open dropdown
        if (openMenuIndex_ >= 0 && openMenuIndex_ < static_cast<int>(menus_.size())) {
            drawDropdown(renderer, openMenuIndex_);
        }
    }

    void MenuBar::drawDropdown(Renderer& renderer, int menuIndex) {
        auto& theme = GetTheme();
        auto& menu = menus_[menuIndex];

        // Calculate dropdown bounds
        dropdownBounds_ = calculateDropdownBounds(menuIndex, renderer);

        // Draw shadow
        Rect shadowBounds = {
            dropdownBounds_.x + 3,
            dropdownBounds_.y + 3,
            dropdownBounds_.w,
            dropdownBounds_.h
        };
        renderer.drawRect(shadowBounds, { 0, 0, 0, 0.3f });

        // Draw dropdown background
        renderer.drawRect(dropdownBounds_, theme.background);
        renderer.drawRectOutline(dropdownBounds_, theme.border);

        // Draw items
        float y = dropdownBounds_.y + DROPDOWN_PADDING;
        for (int i = 0; i < static_cast<int>(menu.items.size()); i++) {
            const auto& item = menu.items[i];

            if (item.separator) {
                float sepY = y + SEPARATOR_HEIGHT / 2;
                renderer.drawRect({
                    dropdownBounds_.x + theme.padding(),
                    sepY,
                    dropdownBounds_.w - theme.padding() * 2,
                    1
                    }, theme.border);
                y += SEPARATOR_HEIGHT;
            }
            else {
                Rect itemBounds = {
                    dropdownBounds_.x,
                    y,
                    dropdownBounds_.w,
                    theme.dropdownItemHeight()
                };

                bool isHovered = (hoveredItemIndex_ == i);
                drawMenuItem(renderer, item, itemBounds, isHovered, item.enabled);
                y += theme.dropdownItemHeight();
            }
        }
    }

    void MenuBar::drawMenuItem(Renderer& renderer, const MenuItem& item, const Rect& itemBounds,
        bool hovered, bool enabled) {
        auto& theme = GetTheme();

        // Hover background
        if (hovered && enabled) {
            renderer.drawRect(itemBounds, theme.accent);
        }

        Color textColor = enabled ? theme.text : theme.textDim;
        float textY = itemBounds.y + (itemBounds.h - theme.fontSize()) / 2;

        // Checkbox/checkmark area
        float contentX = itemBounds.x + theme.padding();
        if (item.checkable) {
            if (item.isChecked()) {
                renderer.drawText("*", contentX, textY, theme.accent, theme.fontSize());
            }
            contentX += CHECKBOX_WIDTH;
        }
        else {
            contentX += ICON_WIDTH;
        }

        // Label
        renderer.drawText(item.label, contentX, textY, textColor, theme.fontSize());

        // Shortcut
        if (!item.shortcut.empty()) {
            Vec2 shortcutSize = renderer.measureText(item.shortcut, theme.fontSize());
            float shortcutX = itemBounds.right() - theme.padding() - shortcutSize.x;
            if (item.hasSubmenu()) {
                shortcutX -= SUBMENU_ARROW_WIDTH;
            }
            Color shortcutColor = enabled ? theme.textDim : theme.textDim;
            renderer.drawText(item.shortcut, shortcutX, textY, shortcutColor, theme.fontSize());
        }

        // Submenu arrow
        if (item.hasSubmenu()) {
            float arrowX = itemBounds.right() - theme.padding() - 8;
            renderer.drawText(">", arrowX, textY, textColor, theme.fontSize());
        }
    }

    Rect MenuBar::calculateDropdownBounds(int menuIndex, Renderer& renderer) {
        auto& theme = GetTheme();
        auto& menu = menus_[menuIndex];

        float width = calculateDropdownWidth(menu, renderer);
        float height = DROPDOWN_PADDING * 2;

        for (const auto& item : menu.items) {
            if (item.separator) {
                height += SEPARATOR_HEIGHT;
            }
            else {
                height += theme.dropdownItemHeight();
            }
        }

        return {
            menu.bounds.x,
            menu.bounds.bottom(),
            width,
            height
        };
    }

    float MenuBar::calculateDropdownWidth(const Menu& menu, Renderer& renderer) {
        auto& theme = GetTheme();
        float maxWidth = MIN_DROPDOWN_WIDTH;

        for (const auto& item : menu.items) {
            if (item.separator) continue;

            float itemWidth = theme.padding() * 2;
            itemWidth += ICON_WIDTH;

            Vec2 labelSize = renderer.measureText(item.label, theme.fontSize());
            itemWidth += labelSize.x;

            if (!item.shortcut.empty()) {
                Vec2 shortcutSize = renderer.measureText(item.shortcut, theme.fontSize());
                itemWidth += SHORTCUT_MIN_GAP + shortcutSize.x;
            }

            if (item.hasSubmenu()) {
                itemWidth += SUBMENU_ARROW_WIDTH;
            }

            maxWidth = std::max(maxWidth, itemWidth);
        }

        return maxWidth;
    }

    bool MenuBar::handleMouse(const MouseEvent& event) {
        auto& theme = GetTheme();
        bool consumed = false;

        // Reset hover states
        for (auto& menu : menus_) {
            menu.hovered = false;
        }

        // Check menu headers
        for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
            if (menus_[i].bounds.contains(event.x, event.y)) {
                menus_[i].hovered = true;

                // If a dropdown is open, hovering opens this one
                if (openMenuIndex_ >= 0 && openMenuIndex_ != i) {
                    openMenuIndex_ = i;
                    hoveredItemIndex_ = -1;
                }

                // Click opens/closes
                if (event.pressed && event.button == MouseButton::Left) {
                    if (openMenuIndex_ == i) {
                        closeDropdown();
                    }
                    else {
                        openMenuIndex_ = i;
                        hoveredItemIndex_ = -1;
                    }
                    return true;
                }

                consumed = true;
            }
        }

        // Handle dropdown interaction
        if (openMenuIndex_ >= 0) {
            auto& menu = menus_[openMenuIndex_];

            if (dropdownBounds_.contains(event.x, event.y)) {
                // Find hovered item
                float y = dropdownBounds_.y + DROPDOWN_PADDING;
                hoveredItemIndex_ = -1;

                for (int i = 0; i < static_cast<int>(menu.items.size()); i++) {
                    const auto& item = menu.items[i];

                    if (item.separator) {
                        y += SEPARATOR_HEIGHT;
                        continue;
                    }

                    Rect itemBounds = {
                        dropdownBounds_.x,
                        y,
                        dropdownBounds_.w,
                        theme.dropdownItemHeight()
                    };

                    if (itemBounds.contains(event.x, event.y)) {
                        hoveredItemIndex_ = i;

                        // Click executes action
                        if (event.pressed && event.button == MouseButton::Left && item.enabled) {
                            if (item.action) {
                                item.action();
                            }
                            closeDropdown();
                            return true;
                        }
                    }

                    y += theme.dropdownItemHeight();
                }

                consumed = true;
            }
            else if (event.pressed) {
                // Click outside closes dropdown
                closeDropdown();
            }
        }

        return consumed || bounds.contains(event.x, event.y);
    }

    bool MenuBar::handleKey(const KeyEvent& event) {
        if (!event.pressed) return false;

        // Escape closes dropdown
        if (event.key == 256 && openMenuIndex_ >= 0) {  // GLFW_KEY_ESCAPE = 256
            closeDropdown();
            return true;
        }

        return false;
    }

    void MenuBar::addMenu(const std::string& label, const std::vector<MenuItem>& items) {
        Menu menu;
        menu.label = label;
        menu.items = items;
        menus_.push_back(menu);
    }

} // namespace LibreUI