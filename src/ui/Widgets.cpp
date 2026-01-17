#include "Widgets.h"

namespace libre::ui {

    // ============================================================================
    // BASE WIDGET
    // ============================================================================

    void Widget::layout(const Rect& available) {
        bounds = available;

        // Layout children in a vertical stack by default
        float y = bounds.y + GetTheme().padding;
        for (auto& child : children_) {
            if (!child->visible) continue;

            Rect childBounds = {
                bounds.x + GetTheme().padding,
                y,
                bounds.w - GetTheme().padding * 2,
                GetTheme().buttonHeight
            };
            child->layout(childBounds);
            y += childBounds.h + GetTheme().spacing;
        }
    }

    void Widget::draw(UIRenderer& renderer) {
        for (auto& child : children_) {
            if (child->visible) {
                child->draw(renderer);
            }
        }
    }

    bool Widget::handleMouse(const MouseEvent& event) {
        // Check children in reverse order (topmost first)
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            if ((*it)->visible && (*it)->handleMouse(event)) {
                return true;
            }
        }
        return false;
    }

    bool Widget::handleKey(const KeyEvent& event) {
        for (auto& child : children_) {
            if (child->handleKey(event)) return true;
        }
        return false;
    }

    void Widget::addChild(std::unique_ptr<Widget> child) {
        child->parent_ = this;
        children_.push_back(std::move(child));
    }

    // ============================================================================
    // LABEL
    // ============================================================================

    Label::Label(const std::string& text) : text(text) {
        color = GetTheme().text;
    }

    void Label::draw(UIRenderer& renderer) {
        renderer.drawText(text, bounds.x, bounds.y, color, fontSize);
    }

    // ============================================================================
    // BUTTON
    // ============================================================================

    Button::Button(const std::string& text) : text(text) {}

    void Button::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        Color bgColor = theme.buttonBackground;
        if (pressed) {
            bgColor = theme.buttonPressed;
        }
        else if (hovered) {
            bgColor = theme.buttonHover;
        }

        renderer.drawRoundedRect(bounds, bgColor, theme.cornerRadius);

        // Center text
        Vec2 textSize = renderer.measureText(text, theme.fontSize);
        float textX = bounds.x + (bounds.w - textSize.x) / 2;
        float textY = bounds.y + (bounds.h - textSize.y) / 2;
        renderer.drawText(text, textX, textY, theme.text, theme.fontSize);
    }

    bool Button::handleMouse(const MouseEvent& event) {
        bool inside = bounds.contains(event.x, event.y);
        hovered = inside;

        if (inside && event.pressed && event.button == MouseButton::Left) {
            pressed = true;
            return true;
        }

        if (pressed && event.released && event.button == MouseButton::Left) {
            pressed = false;
            if (inside && onClick) {
                onClick();
            }
            return true;
        }

        return inside;
    }

    // ============================================================================
    // PANEL
    // ============================================================================

    Panel::Panel(const std::string& title) : title(title) {}

    void Panel::layout(const Rect& available) {
        bounds = available;
        auto& theme = GetTheme();

        headerBounds_ = { bounds.x, bounds.y, bounds.w, theme.panelHeaderHeight };

        if (collapsed) {
            contentBounds_ = { 0, 0, 0, 0 };
        }
        else {
            contentBounds_ = {
                bounds.x,
                bounds.y + theme.panelHeaderHeight,
                bounds.w,
                bounds.h - theme.panelHeaderHeight
            };

            // Layout children in content area
            float y = contentBounds_.y + theme.padding;
            for (auto& child : children_) {
                if (!child->visible) continue;

                Rect childBounds = {
                    contentBounds_.x + theme.padding,
                    y,
                    contentBounds_.w - theme.padding * 2,
                    theme.buttonHeight
                };
                child->layout(childBounds);
                y += childBounds.h + theme.spacing;
            }
        }
    }

    void Panel::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Header
        Color headerColor = hovered ? theme.panelHeaderHover : theme.panelHeader;
        renderer.drawRoundedRect(headerBounds_, headerColor, theme.cornerRadius);

        // Collapse indicator
        std::string indicator = collapsed ? ">" : "v";
        renderer.drawText(indicator, headerBounds_.x + 8, headerBounds_.y + 6,
            theme.text, theme.fontSize);

        // Title
        renderer.drawText(title, headerBounds_.x + 24, headerBounds_.y + 6,
            theme.text, theme.fontSize);

        // Content
        if (!collapsed && contentBounds_.h > 0) {
            renderer.drawRect(contentBounds_, theme.background);

            // Draw children
            renderer.pushClip(contentBounds_);
            Widget::draw(renderer);
            renderer.popClip();
        }
    }

    bool Panel::handleMouse(const MouseEvent& event) {
        hovered = headerBounds_.contains(event.x, event.y);

        // Header click toggles collapse
        if (collapsible && hovered && event.pressed && event.button == MouseButton::Left) {
            collapsed = !collapsed;
            return true;
        }

        // Pass to children if not collapsed
        if (!collapsed && contentBounds_.contains(event.x, event.y)) {
            return Widget::handleMouse(event);
        }

        return hovered;
    }

    // ============================================================================
    // DROPDOWN
    // ============================================================================

    Dropdown::Dropdown() {}

    void Dropdown::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Main button area
        Color bgColor = hovered ? theme.buttonHover : theme.buttonBackground;
        renderer.drawRoundedRect(bounds, bgColor, theme.cornerRadius);

        // Selected text
        std::string displayText = items.empty() ? "" : items[selectedIndex];
        renderer.drawText(displayText, bounds.x + theme.padding,
            bounds.y + (bounds.h - theme.fontSize) / 2,
            theme.text, theme.fontSize);

        // Dropdown arrow
        renderer.drawText("v", bounds.right() - 16,
            bounds.y + (bounds.h - theme.fontSize) / 2,
            theme.textDim, theme.fontSize);

        // Dropdown list when open
        if (open && !items.empty()) {
            Rect dropBounds = getDropdownBounds();

            // Background
            renderer.drawRect(dropBounds, theme.dropdownBackground);
            renderer.drawRectOutline(dropBounds, theme.border);

            // Items
            float y = dropBounds.y;
            for (int i = 0; i < static_cast<int>(items.size()); i++) {
                Rect itemBounds = { dropBounds.x, y, dropBounds.w, theme.dropdownItemHeight };

                if (i == hoveredItem) {
                    renderer.drawRect(itemBounds, theme.dropdownItemHover);
                }

                renderer.drawText(items[i], itemBounds.x + theme.padding,
                    itemBounds.y + (itemBounds.h - theme.fontSize) / 2,
                    theme.text, theme.fontSize);

                y += theme.dropdownItemHeight;
            }
        }
    }

    bool Dropdown::handleMouse(const MouseEvent& event) {
        auto& theme = GetTheme();
        bool insideMain = bounds.contains(event.x, event.y);
        hovered = insideMain;

        if (open) {
            Rect dropBounds = getDropdownBounds();
            bool insideDrop = dropBounds.contains(event.x, event.y);

            // Update hovered item
            if (insideDrop) {
                hoveredItem = static_cast<int>((event.y - dropBounds.y) / theme.dropdownItemHeight);
                if (hoveredItem >= static_cast<int>(items.size())) {
                    hoveredItem = -1;
                }
            }
            else {
                hoveredItem = -1;
            }

            if (event.pressed && event.button == MouseButton::Left) {
                if (insideDrop && hoveredItem >= 0) {
                    selectedIndex = hoveredItem;
                    if (onSelect) onSelect(selectedIndex);
                }
                open = false;
                return true;
            }

            return insideMain || insideDrop;
        }

        // Open on click
        if (insideMain && event.pressed && event.button == MouseButton::Left) {
            open = true;
            return true;
        }

        return insideMain;
    }

    Rect Dropdown::getDropdownBounds() const {
        auto& theme = GetTheme();
        float height = items.size() * theme.dropdownItemHeight;
        return { bounds.x, bounds.bottom(), bounds.w, height };
    }

    // ============================================================================
    // MENU BAR
    // ============================================================================

    MenuBar::MenuBar() {}

    void MenuBar::layout(const Rect& available) {
        bounds = available;
        auto& theme = GetTheme();

        // Calculate menu bounds
        float x = bounds.x;
        for (auto& menu : menus_) {
            Vec2 textSize = { menu.label.length() * 8.0f, theme.fontSize };  // Approximate
            menu.bounds = { x, bounds.y, textSize.x + theme.padding * 2, bounds.h };
            x += menu.bounds.w;
        }
    }

    void MenuBar::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Background
        renderer.drawRect(bounds, theme.backgroundDark);

        // Menu items
        for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
            auto& menu = menus_[i];

            bool isOpen = (openMenuIndex_ == i);
            bool isHovered = menu.bounds.contains(
                renderer.getScreenWidth() * 0.5f, renderer.getScreenHeight() * 0.5f);  // Simplified

            if (isOpen) {
                renderer.drawRect(menu.bounds, theme.accent);
            }

            renderer.drawText(menu.label,
                menu.bounds.x + theme.padding,
                menu.bounds.y + (menu.bounds.h - theme.fontSize) / 2,
                theme.text, theme.fontSize);

            // Draw dropdown if open
            if (isOpen && !menu.items.empty()) {
                Rect dropBounds = getMenuDropdownBounds(i);
                renderer.drawRect(dropBounds, theme.dropdownBackground);
                renderer.drawRectOutline(dropBounds, theme.border);

                float y = dropBounds.y;
                for (int j = 0; j < static_cast<int>(menu.items.size()); j++) {
                    auto& item = menu.items[j];
                    Rect itemBounds = { dropBounds.x, y, dropBounds.w, theme.dropdownItemHeight };

                    if (item.separator) {
                        float lineY = itemBounds.y + itemBounds.h / 2;
                        renderer.drawRect({ itemBounds.x + 4, lineY, itemBounds.w - 8, 1 },
                            theme.border);
                    }
                    else {
                        if (j == hoveredItemIndex_) {
                            renderer.drawRect(itemBounds, theme.dropdownItemHover);
                        }
                        renderer.drawText(item.label, itemBounds.x + theme.padding,
                            itemBounds.y + (itemBounds.h - theme.fontSize) / 2,
                            theme.text, theme.fontSize);
                    }

                    y += theme.dropdownItemHeight;
                }
            }
        }
    }

    bool MenuBar::handleMouse(const MouseEvent& event) {
        auto& theme = GetTheme();

        // Check menu headers
        for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
            if (menus_[i].bounds.contains(event.x, event.y)) {
                if (event.pressed && event.button == MouseButton::Left) {
                    openMenuIndex_ = (openMenuIndex_ == i) ? -1 : i;
                    return true;
                }
                return true;
            }
        }

        // Check open dropdown
        if (openMenuIndex_ >= 0) {
            Rect dropBounds = getMenuDropdownBounds(openMenuIndex_);

            if (dropBounds.contains(event.x, event.y)) {
                // Find hovered item
                auto& menu = menus_[openMenuIndex_];
                hoveredItemIndex_ = static_cast<int>((event.y - dropBounds.y) / theme.dropdownItemHeight);

                if (hoveredItemIndex_ >= static_cast<int>(menu.items.size())) {
                    hoveredItemIndex_ = -1;
                }

                if (event.pressed && event.button == MouseButton::Left && hoveredItemIndex_ >= 0) {
                    auto& item = menu.items[hoveredItemIndex_];
                    if (!item.separator && item.action) {
                        item.action();
                    }
                    openMenuIndex_ = -1;
                    return true;
                }

                return true;
            }

            // Click outside closes menu
            if (event.pressed) {
                openMenuIndex_ = -1;
            }
        }

        return bounds.contains(event.x, event.y);
    }

    void MenuBar::addMenu(const std::string& label, const std::vector<MenuItem>& items) {
        Menu menu;
        menu.label = label;
        menu.items = items;
        menus_.push_back(menu);
    }

    Rect MenuBar::getMenuDropdownBounds(int menuIndex) const {
        if (menuIndex < 0 || menuIndex >= static_cast<int>(menus_.size())) {
            return {};
        }

        auto& theme = GetTheme();
        auto& menu = menus_[menuIndex];

        float maxWidth = 150.0f;  // Minimum width
        for (auto& item : menu.items) {
            float w = item.label.length() * 8.0f + theme.padding * 2;
            if (w > maxWidth) maxWidth = w;
        }

        float height = menu.items.size() * theme.dropdownItemHeight;

        return { menu.bounds.x, menu.bounds.bottom(), maxWidth, height };
    }

    // ============================================================================
    // WINDOW
    // ============================================================================

    Window::Window(const std::string& title) : title(title) {}

    void Window::layout(const Rect& available) {
        // Window uses its own bounds, not available
        auto& theme = GetTheme();

        titleBarBounds_ = { bounds.x, bounds.y, bounds.w, theme.panelHeaderHeight };
        closeButtonBounds_ = { bounds.right() - 24, bounds.y + 4, 18, 18 };
        contentBounds_ = {
            bounds.x,
            bounds.y + theme.panelHeaderHeight,
            bounds.w,
            bounds.h - theme.panelHeaderHeight
        };

        // Layout children in content area
        float y = contentBounds_.y + theme.padding;
        for (auto& child : children_) {
            if (!child->visible) continue;

            Rect childBounds = {
                contentBounds_.x + theme.padding,
                y,
                contentBounds_.w - theme.padding * 2,
                theme.buttonHeight
            };
            child->layout(childBounds);
            y += childBounds.h + theme.spacing;
        }
    }

    void Window::draw(UIRenderer& renderer) {
        if (!isOpen) return;

        auto& theme = GetTheme();

        // Shadow (simple offset rectangle)
        Rect shadowBounds = { bounds.x + 4, bounds.y + 4, bounds.w, bounds.h };
        renderer.drawRect(shadowBounds, { 0, 0, 0, 0.3f });

        // Background
        renderer.drawRect(bounds, theme.background);
        renderer.drawRectOutline(bounds, theme.border);

        // Title bar
        renderer.drawRect(titleBarBounds_, theme.panelHeader);
        renderer.drawText(title, titleBarBounds_.x + theme.padding,
            titleBarBounds_.y + (titleBarBounds_.h - theme.fontSize) / 2,
            theme.text, theme.fontSize);

        // Close button
        if (closable) {
            Color closeColor = closeHovered_ ? theme.accentHover : theme.textDim;
            renderer.drawText("X", closeButtonBounds_.x + 4, closeButtonBounds_.y + 2,
                closeColor, theme.fontSize);
        }

        // Content
        renderer.pushClip(contentBounds_);
        Widget::draw(renderer);
        renderer.popClip();
    }

    bool Window::handleMouse(const MouseEvent& event) {
        if (!isOpen) return false;

        // Close button
        if (closable) {
            closeHovered_ = closeButtonBounds_.contains(event.x, event.y);
            if (closeHovered_ && event.pressed && event.button == MouseButton::Left) {
                isOpen = false;
                if (onClose) onClose();
                return true;
            }
        }

        // Title bar dragging
        if (draggable) {
            if (titleBarBounds_.contains(event.x, event.y) && !closeButtonBounds_.contains(event.x, event.y)) {
                if (event.pressed && event.button == MouseButton::Left) {
                    dragging_ = true;
                    dragOffsetX_ = event.x - bounds.x;
                    dragOffsetY_ = event.y - bounds.y;
                    return true;
                }
            }

            if (dragging_) {
                if (event.released) {
                    dragging_ = false;
                }
                else {
                    bounds.x = event.x - dragOffsetX_;
                    bounds.y = event.y - dragOffsetY_;
                    layout(bounds);  // Recalculate internal bounds
                }
                return true;
            }
        }

        // Content
        if (contentBounds_.contains(event.x, event.y)) {
            return Widget::handleMouse(event);
        }

        return bounds.contains(event.x, event.y);
    }

} // namespace libre::ui