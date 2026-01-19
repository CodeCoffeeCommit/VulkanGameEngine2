// src/ui/Widgets.cpp
// Updated to use scaled theme getters instead of direct member access

#include "Widgets.h"
#include <algorithm>
#include <cstdio>
#include <iostream>

namespace libre::ui {

    // Helper function for clamping
    template<typename T>
    T clampVal(T val, T minV, T maxV) {
        return std::max(minV, std::min(maxV, val));
    }

    // ============================================================================
    // BASE WIDGET
    // ============================================================================

    void Widget::layout(const Rect& available) {
        bounds = available;
        for (auto& child : children_) {
            child->layout(available);
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
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            if ((*it)->visible && (*it)->handleMouse(event)) {
                return true;
            }
        }
        return false;
    }

    bool Widget::handleKey(const KeyEvent& event) {
        for (auto& child : children_) {
            if (child->handleKey(event)) {
                return true;
            }
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

        renderer.drawRoundedRect(bounds, bgColor, theme.cornerRadius());

        // Center text
        Vec2 textSize = renderer.measureText(text, theme.fontSize());
        float textX = bounds.x + (bounds.w - textSize.x) / 2;
        float textY = bounds.y + (bounds.h - textSize.y) / 2;
        renderer.drawText(text, textX, textY, theme.text, theme.fontSize());
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

        headerBounds_ = { bounds.x, bounds.y, bounds.w, theme.panelHeaderHeight() };

        if (collapsed) {
            contentBounds_ = { 0, 0, 0, 0 };
        }
        else {
            contentBounds_ = {
                bounds.x,
                bounds.y + theme.panelHeaderHeight(),
                bounds.w,
                bounds.h - theme.panelHeaderHeight()
            };

            // Layout children in content area
            float y = contentBounds_.y + theme.padding();
            for (auto& child : children_) {
                if (!child->visible) continue;

                Rect childBounds = {
                    contentBounds_.x + theme.padding(),
                    y,
                    contentBounds_.w - theme.padding() * 2,
                    theme.buttonHeight()
                };
                child->layout(childBounds);
                y += childBounds.h + theme.spacing();
            }
        }
    }

    void Panel::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Header
        Color headerColor = hovered ? theme.panelHeaderHover : theme.panelHeader;
        renderer.drawRoundedRect(headerBounds_, headerColor, theme.cornerRadius());

        // Collapse indicator
        std::string indicator = collapsed ? ">" : "v";
        renderer.drawText(indicator, headerBounds_.x + 8, headerBounds_.y + 6,
            theme.text, theme.fontSize());

        // Title
        renderer.drawText(title, headerBounds_.x + 24, headerBounds_.y + 6,
            theme.text, theme.fontSize());

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
        renderer.drawRoundedRect(bounds, bgColor, theme.cornerRadius());

        // Selected text
        std::string displayText = items.empty() ? ""
            : (selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size()))
            ? items[selectedIndex] : items[0];

        renderer.drawText(displayText, bounds.x + theme.padding(),
            bounds.y + (bounds.h - theme.fontSize()) / 2,
            theme.text, theme.fontSize());

        // Arrow
        renderer.drawText("v", bounds.right() - 16,
            bounds.y + (bounds.h - theme.fontSize()) / 2,
            theme.textDim, theme.fontSize());

        // Dropdown menu if open
        if (open && !items.empty()) {
            Rect dropBounds = getDropdownBounds();
            renderer.drawRect(dropBounds, theme.dropdownBackground);
            renderer.drawRectOutline(dropBounds, theme.border);

            float y = dropBounds.y;
            for (int i = 0; i < static_cast<int>(items.size()); i++) {
                Rect itemBounds = { dropBounds.x, y, dropBounds.w, theme.dropdownItemHeight() };

                if (i == hoveredItem) {
                    renderer.drawRect(itemBounds, theme.dropdownItemHover);
                }

                renderer.drawText(items[i], itemBounds.x + theme.padding(),
                    itemBounds.y + (itemBounds.h - theme.fontSize()) / 2,
                    theme.text, theme.fontSize());

                y += theme.dropdownItemHeight();
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
                hoveredItem = static_cast<int>((event.y - dropBounds.y) / theme.dropdownItemHeight());
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
        float height = items.size() * theme.dropdownItemHeight();
        return { bounds.x, bounds.bottom(), bounds.w, height };
    }

    // ============================================================================
    // MENU BAR
    // ============================================================================

    MenuBar::MenuBar() {}

    void MenuBar::layout(const Rect& available) {
        bounds = available;
        auto& theme = GetTheme();

        // Calculate menu header bounds
        float x = bounds.x + theme.padding();
        for (auto& menu : menus_) {
            float textWidth = menu.label.length() * 7.0f;  // Approximate width
            menu.bounds = {
                x,
                bounds.y,
                textWidth + theme.padding() * 2,
                bounds.h
            };
            x += menu.bounds.w;
        }
    }

    void MenuBar::draw(UIRenderer& renderer) {
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
            Color textColor = theme.text;
            float textX = menu.bounds.x + theme.padding();
            float textY = menu.bounds.y + (menu.bounds.h - theme.fontSize()) / 2;
            renderer.drawText(menu.label, textX, textY, textColor, theme.fontSize());
        }

        // Draw open dropdown
        if (openMenuIndex_ >= 0 && openMenuIndex_ < static_cast<int>(menus_.size())) {
            drawDropdown(renderer, openMenuIndex_);
        }
    }

    void MenuBar::drawDropdown(UIRenderer& renderer, int menuIndex) {
        auto& theme = GetTheme();
        auto& menu = menus_[menuIndex];

        // Calculate dropdown bounds
        dropdownBounds_ = calculateDropdownBounds(menuIndex, renderer);

        // Draw dropdown shadow
        Rect shadowBounds = {
            dropdownBounds_.x + 3,
            dropdownBounds_.y + 3,
            dropdownBounds_.w,
            dropdownBounds_.h
        };
        renderer.drawRect(shadowBounds, { 0, 0, 0, 0.3f });

        // Draw dropdown background
        renderer.drawRect(dropdownBounds_, theme.dropdownBackground);
        renderer.drawRectOutline(dropdownBounds_, theme.border);

        // Draw items
        float y = dropdownBounds_.y + DROPDOWN_PADDING;
        float itemHeight = theme.dropdownItemHeight();

        for (int i = 0; i < static_cast<int>(menu.items.size()); i++) {
            auto& item = menu.items[i];

            if (item.separator) {
                // Draw separator
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
                    itemHeight
                };

                bool isHovered = (i == hoveredItemIndex_);
                drawMenuItem(renderer, item, itemBounds, isHovered, item.enabled);
                y += itemHeight;
            }
        }
    }

    void MenuBar::drawMenuItem(UIRenderer& renderer, const MenuItem& item,
        const Rect& itemBounds, bool hovered, bool enabled) {
        auto& theme = GetTheme();

        // Hover highlight
        if (hovered && enabled) {
            renderer.drawRect(itemBounds, theme.accent);
        }

        // Text color
        Color textColor = enabled ? theme.text : theme.textDim;
        if (hovered && enabled) {
            textColor = theme.text;  // Keep white on accent background
        }

        float x = itemBounds.x + theme.padding();
        float textY = itemBounds.y + (itemBounds.h - theme.fontSize()) / 2;

        // Checkbox/checkmark area
        if (item.checkable) {
            if (item.isChecked()) {
                // Draw checkmark
                float checkX = x + 2;
                float checkY = itemBounds.y + (itemBounds.h - theme.fontSize()) / 2;
                renderer.drawText("*", checkX, checkY, textColor, theme.fontSize());
            }
            x += CHECKBOX_WIDTH;
        }
        else {
            // Icon area (placeholder for future)
            x += ICON_WIDTH;
        }

        // Label
        renderer.drawText(item.label, x, textY, textColor, theme.fontSize());

        // Shortcut (right-aligned)
        if (!item.shortcut.empty()) {
            Vec2 shortcutSize = renderer.measureText(item.shortcut, theme.fontSize());
            float shortcutX = itemBounds.right() - shortcutSize.x - theme.padding();
            Color shortcutColor = enabled ? theme.textDim : theme.textDim;
            renderer.drawText(item.shortcut, shortcutX, textY, shortcutColor, theme.fontSize());
        }

        // Submenu arrow
        if (item.hasSubmenu()) {
            float arrowX = itemBounds.right() - theme.padding() - 8;
            renderer.drawText(">", arrowX, textY, textColor, theme.fontSize());
        }
    }

    Rect MenuBar::calculateDropdownBounds(int menuIndex, UIRenderer& renderer) {
        auto& theme = GetTheme();
        auto& menu = menus_[menuIndex];

        float width = calculateDropdownWidth(menu, renderer);
        float height = DROPDOWN_PADDING * 2;  // Top and bottom padding

        for (const auto& item : menu.items) {
            if (item.separator) {
                height += SEPARATOR_HEIGHT;
            }
            else {
                height += theme.dropdownItemHeight();
            }
        }

        return {
            menu.bounds.x,           // Align with left edge of header
            menu.bounds.bottom(),    // No gap - directly below header
            width,
            height
        };
    }

    float MenuBar::calculateDropdownWidth(const Menu& menu, UIRenderer& renderer) {
        auto& theme = GetTheme();
        float maxWidth = MIN_DROPDOWN_WIDTH;

        for (const auto& item : menu.items) {
            if (item.separator) continue;

            // Calculate width needed for this item
            float itemWidth = theme.padding() * 2;  // Left and right padding
            itemWidth += ICON_WIDTH;                 // Icon/checkbox area

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

                // If a dropdown is already open, switch to this one on hover
                if (openMenuIndex_ >= 0 && openMenuIndex_ != i) {
                    openMenuIndex_ = i;
                    hoveredItemIndex_ = -1;
                }

                // Click to toggle dropdown
                if (event.pressed && event.button == MouseButton::Left) {
                    if (openMenuIndex_ == i) {
                        openMenuIndex_ = -1;  // Close
                    }
                    else {
                        openMenuIndex_ = i;   // Open
                    }
                    hoveredItemIndex_ = -1;
                    return true;
                }

                consumed = true;
            }
        }

        // Handle open dropdown
        if (openMenuIndex_ >= 0) {
            auto& menu = menus_[openMenuIndex_];

            if (dropdownBounds_.contains(event.x, event.y)) {
                // Find which item is hovered
                float y = dropdownBounds_.y + DROPDOWN_PADDING;
                float itemHeight = theme.dropdownItemHeight();
                int newHoveredIndex = -1;

                for (int i = 0; i < static_cast<int>(menu.items.size()); i++) {
                    auto& item = menu.items[i];
                    float h = item.separator ? SEPARATOR_HEIGHT : itemHeight;

                    if (event.y >= y && event.y < y + h && !item.separator) {
                        newHoveredIndex = i;
                    }
                    y += h;
                }

                hoveredItemIndex_ = newHoveredIndex;

                // Click on item
                if (event.pressed && event.button == MouseButton::Left &&
                    hoveredItemIndex_ >= 0) {

                    auto& item = menu.items[hoveredItemIndex_];
                    if (item.enabled && !item.separator && !item.hasSubmenu()) {
                        if (item.action) {
                            item.action();
                        }
                        closeDropdown();
                        return true;
                    }
                }

                return true;  // Consume event even if no item clicked
            }
            else {
                // Click outside dropdown - close it
                if (event.pressed && event.button == MouseButton::Left) {
                    // But not if clicking on menu bar
                    if (!bounds.contains(event.x, event.y)) {
                        closeDropdown();
                    }
                }
                hoveredItemIndex_ = -1;
            }
        }

        return consumed || bounds.contains(event.x, event.y);
    }

    bool MenuBar::handleKey(const KeyEvent& event) {
        if (!event.pressed) return false;

        // Escape closes dropdown
        if (event.key == 256 && openMenuIndex_ >= 0) {  // GLFW_KEY_ESCAPE
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

    // ============================================================================
    // WINDOW
    // ============================================================================

    Window::Window(const std::string& title) : title(title) {}

    void Window::layout(const Rect& available) {
        auto& theme = GetTheme();

        titleBarBounds_ = { bounds.x, bounds.y, bounds.w, theme.panelHeaderHeight() };
        closeButtonBounds_ = { bounds.right() - 24, bounds.y + 4, 18, 18 };
        contentBounds_ = {
            bounds.x,
            bounds.y + theme.panelHeaderHeight(),
            bounds.w,
            bounds.h - theme.panelHeaderHeight()
        };

        // Layout children in content area
        float y = contentBounds_.y + theme.padding();
        for (auto& child : children_) {
            if (!child->visible) continue;

            Rect childBounds = {
                contentBounds_.x + theme.padding(),
                y,
                contentBounds_.w - theme.padding() * 2,
                theme.buttonHeight()
            };
            child->layout(childBounds);
            y += childBounds.h + theme.spacing();
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
        renderer.drawText(title, titleBarBounds_.x + theme.padding(),
            titleBarBounds_.y + (titleBarBounds_.h - theme.fontSize()) / 2,
            theme.text, theme.fontSize());

        // Close button
        if (closable) {
            Color closeColor = closeHovered_ ? theme.accentHover : theme.textDim;
            renderer.drawText("X", closeButtonBounds_.x + 4, closeButtonBounds_.y + 2,
                closeColor, theme.fontSize());
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
                    layout(bounds);
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

    // ============================================================================
    // CHECKBOX
    // ============================================================================

    Checkbox::Checkbox(const std::string& label) : label(label) {}

    void Checkbox::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Calculate box position (left side)
        boxBounds_ = { bounds.x, bounds.y + (bounds.h - BOX_SIZE) / 2, BOX_SIZE, BOX_SIZE };

        // Draw box background
        Color boxBg = hovered ? theme.buttonHover : theme.buttonBackground;
        renderer.drawRoundedRect(boxBounds_, boxBg, 2.0f);
        renderer.drawRectOutline(boxBounds_, theme.border);

        // Draw checkmark if checked
        if (checked) {
            Rect inner = boxBounds_.shrink(3.0f);
            renderer.drawRoundedRect(inner, theme.accent, 2.0f);
        }

        // Draw label
        float labelX = boxBounds_.right() + theme.padding();
        float labelY = bounds.y + (bounds.h - theme.fontSize()) / 2;
        renderer.drawText(label, labelX, labelY, theme.text, theme.fontSize());
    }

    bool Checkbox::handleMouse(const MouseEvent& event) {
        bool inside = bounds.contains(event.x, event.y);
        hovered = inside;

        if (inside && event.pressed && event.button == MouseButton::Left) {
            checked = !checked;
            if (onChange) onChange(checked);
            return true;
        }

        return inside;
    }

    // ============================================================================
    // SLIDER
    // ============================================================================

    Slider::Slider(float minVal, float maxVal)
        : minValue(minVal), maxValue(maxVal), value((minVal + maxVal) / 2) {
    }

    void Slider::layout(const Rect& available) {
        bounds = available;

        // Track is in the middle portion (after label, before value display)
        float trackX = bounds.x + LABEL_WIDTH;
        float trackW = bounds.w - LABEL_WIDTH - (showValue ? VALUE_WIDTH : 0);
        float trackY = bounds.y + (bounds.h - TRACK_HEIGHT) / 2;

        trackBounds_ = { trackX, trackY, trackW, TRACK_HEIGHT };

        // Handle position based on value
        float handleX = valueToPosition(value) - HANDLE_WIDTH / 2;
        handleBounds_ = { handleX, bounds.y + 2, HANDLE_WIDTH, bounds.h - 4 };
    }

    void Slider::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Draw label
        renderer.drawText(label, bounds.x, bounds.y + (bounds.h - theme.fontSize()) / 2,
            theme.text, theme.fontSize());

        // Draw track background
        renderer.drawRoundedRect(trackBounds_, theme.backgroundDark, TRACK_HEIGHT / 2);

        // Draw filled portion
        Rect filled = { trackBounds_.x, trackBounds_.y,
                       valueToPosition(value) - trackBounds_.x, trackBounds_.h };
        renderer.drawRoundedRect(filled, theme.accent, TRACK_HEIGHT / 2);

        // Draw handle
        Color handleColor = (hovered || dragging_) ? theme.accentHover : theme.buttonBackground;
        renderer.drawRoundedRect(handleBounds_, handleColor, 4.0f);
        renderer.drawRectOutline(handleBounds_, theme.border);

        // Draw value
        if (showValue) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.*f", precision, value);
            float valueX = trackBounds_.right() + theme.padding();
            renderer.drawText(buf, valueX, bounds.y + (bounds.h - theme.fontSize()) / 2,
                theme.textDim, theme.fontSize());
        }
    }

    bool Slider::handleMouse(const MouseEvent& event) {
        bool insideTrack = trackBounds_.contains(event.x, event.y);
        bool insideHandle = handleBounds_.contains(event.x, event.y);
        hovered = insideTrack || insideHandle;

        if ((insideTrack || insideHandle) && event.pressed && event.button == MouseButton::Left) {
            dragging_ = true;
            value = positionToValue(event.x);
            if (onChange) onChange(value);
            return true;
        }

        if (dragging_) {
            if (event.released) {
                dragging_ = false;
            }
            else {
                value = positionToValue(event.x);
                if (onChange) onChange(value);
            }
            return true;
        }

        return hovered;
    }

    float Slider::valueToPosition(float val) const {
        float t = (val - minValue) / (maxValue - minValue);
        t = std::clamp(t, 0.0f, 1.0f);
        return trackBounds_.x + t * trackBounds_.w;
    }

    float Slider::positionToValue(float x) const {
        float t = (x - trackBounds_.x) / trackBounds_.w;
        t = std::clamp(t, 0.0f, 1.0f);
        return minValue + t * (maxValue - minValue);
    }

    // ============================================================================
    // TEXT FIELD
    // ============================================================================

    TextField::TextField(const std::string& placeholder) : placeholder(placeholder) {}

    void TextField::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Background
        Color bgColor = focused_ ? theme.backgroundLight : theme.buttonBackground;
        renderer.drawRoundedRect(bounds, bgColor, theme.cornerRadius());

        // Border (highlight when focused)
        Color borderColor = focused_ ? theme.accent : theme.border;
        renderer.drawRectOutline(bounds, borderColor);

        // Text content area
        Rect textArea = bounds.shrink(theme.padding());
        renderer.pushClip(textArea);

        // Draw text or placeholder
        std::string displayText;
        Color textColor;

        if (text.empty() && !focused_) {
            displayText = placeholder;
            textColor = theme.textDim;
        }
        else {
            displayText = password ? std::string(text.length(), '*') : text;
            textColor = theme.text;
        }

        float textY = bounds.y + (bounds.h - theme.fontSize()) / 2;
        renderer.drawText(displayText, textArea.x, textY, textColor, theme.fontSize());

        // Cursor (when focused)
        if (focused_) {
            float cursorX = textArea.x + renderer.measureText(displayText.substr(0, cursorPos_), theme.fontSize()).x;
            renderer.drawRect({ cursorX, textArea.y + 2, 1, textArea.h - 4 }, theme.text);
        }

        renderer.popClip();
    }

    bool TextField::handleMouse(const MouseEvent& event) {
        bool inside = bounds.contains(event.x, event.y);

        if (event.pressed && event.button == MouseButton::Left) {
            focused_ = inside;
            return inside;
        }

        return inside;
    }

    bool TextField::handleKey(const KeyEvent& event) {
        if (!focused_ || !event.pressed) return false;

        // Basic text input handling would go here
        // For now, just return true to indicate we handled it
        return true;
    }

    // ============================================================================
    // SEPARATOR
    // ============================================================================

    Separator::Separator(bool horizontal) : horizontal(horizontal) {}

    void Separator::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        if (horizontal) {
            float y = bounds.y + bounds.h / 2;
            renderer.drawRect({ bounds.x, y, bounds.w, 1 }, theme.border);
        }
        else {
            float x = bounds.x + bounds.w / 2;
            renderer.drawRect({ x, bounds.y, 1, bounds.h }, theme.border);
        }
    }

    // ============================================================================
    // SCROLL AREA
    // ============================================================================

    ScrollArea::ScrollArea() {}

    void ScrollArea::layout(const Rect& available) {
        bounds = available;
        auto& theme = GetTheme();

        // Calculate total content height
        contentHeight_ = 0;
        for (auto& child : children_) {
            if (child->visible) {
                contentHeight_ += theme.buttonHeight() + theme.spacing();
            }
        }

        maxScroll_ = std::max(0.0f, contentHeight_ - bounds.h);
        scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll_);

        // Layout children with scroll offset
        float y = bounds.y - scrollOffset + theme.padding();
        for (auto& child : children_) {
            if (!child->visible) continue;

            Rect childBounds = {
                bounds.x + theme.padding(),
                y,
                bounds.w - theme.padding() * 2 - (showScrollbar && maxScroll_ > 0 ? 12.0f : 0),
                theme.buttonHeight()
            };
            child->layout(childBounds);
            y += childBounds.h + theme.spacing();
        }

        // Scrollbar bounds
        if (showScrollbar && maxScroll_ > 0) {
            scrollbarBounds_ = { bounds.right() - 10, bounds.y, 8, bounds.h };

            float thumbHeight = (bounds.h / contentHeight_) * bounds.h;
            thumbHeight = std::max(thumbHeight, 20.0f);
            float thumbY = bounds.y + (scrollOffset / maxScroll_) * (bounds.h - thumbHeight);
            thumbBounds_ = { scrollbarBounds_.x, thumbY, scrollbarBounds_.w, thumbHeight };
        }
    }

    void ScrollArea::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Clip to bounds
        renderer.pushClip(bounds);

        // Draw children
        for (auto& child : children_) {
            if (child->visible) {
                // Only draw if visible in viewport
                if (child->bounds.bottom() > bounds.y && child->bounds.y < bounds.bottom()) {
                    child->draw(renderer);
                }
            }
        }

        renderer.popClip();

        // Draw scrollbar
        if (showScrollbar && maxScroll_ > 0) {
            renderer.drawRoundedRect(scrollbarBounds_, theme.backgroundDark, 4.0f);

            Color thumbColor = scrollbarDragging_ ? theme.accent : theme.scrollbarThumb;
            renderer.drawRoundedRect(thumbBounds_, thumbColor, 4.0f);
        }
    }

    bool ScrollArea::handleMouse(const MouseEvent& event) {
        // Handle scroll wheel
        if (bounds.contains(event.x, event.y) && event.scroll != 0) {
            scrollOffset -= event.scroll * 30.0f;
            scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll_);
            return true;
        }

        // Handle scrollbar dragging
        if (showScrollbar && maxScroll_ > 0) {
            if (thumbBounds_.contains(event.x, event.y) && event.pressed && event.button == MouseButton::Left) {
                scrollbarDragging_ = true;
                dragStartY_ = event.y;
                dragStartOffset_ = scrollOffset;
                return true;
            }

            if (scrollbarDragging_) {
                if (event.released) {
                    scrollbarDragging_ = false;
                }
                else {
                    float delta = event.y - dragStartY_;
                    float scrollRange = bounds.h - thumbBounds_.h;
                    if (scrollRange > 0) {
                        scrollOffset = dragStartOffset_ + (delta / scrollRange) * maxScroll_;
                        scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll_);
                    }
                }
                return true;
            }
        }

        // Pass to children
        if (bounds.contains(event.x, event.y)) {
            return Widget::handleMouse(event);
        }

        return false;
    }

} // namespace libre::ui