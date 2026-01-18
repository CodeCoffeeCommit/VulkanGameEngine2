#include "Widgets.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

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
        std::cout << "[DEBUG] MenuBar::draw bounds: x=" << bounds.x
            << " y=" << bounds.y
            << " w=" << bounds.w
            << " h=" << bounds.h
            << " menus=" << menus_.size() << std::endl;
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


    // ============================================================================
// ADD THESE TO Widgets.cpp (after existing implementations)
// ============================================================================

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
        float labelX = boxBounds_.right() + theme.padding;
        float labelY = bounds.y + (bounds.h - theme.fontSize) / 2;
        renderer.drawText(label, labelX, labelY, theme.text, theme.fontSize);
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
        auto& theme = GetTheme();

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
        renderer.drawText(label, bounds.x, bounds.y + (bounds.h - theme.fontSize) / 2,
            theme.text, theme.fontSize);

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
            float valueX = trackBounds_.right() + theme.padding;
            renderer.drawText(buf, valueX, bounds.y + (bounds.h - theme.fontSize) / 2,
                theme.textDim, theme.fontSize);
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
        renderer.drawRoundedRect(bounds, bgColor, theme.cornerRadius);

        // Border (highlight when focused)
        Color borderColor = focused_ ? theme.accent : theme.border;
        renderer.drawRectOutline(bounds, borderColor);

        // Text content area
        Rect textArea = bounds.shrink(theme.padding);
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

        float textY = textArea.y + (textArea.h - theme.fontSize) / 2;
        renderer.drawText(displayText, textArea.x, textY, textColor, theme.fontSize);

        // Draw cursor when focused
        if (focused_ && cursorVisible_) {
            std::string textBeforeCursor = displayText.substr(0, cursorPos_);
            Vec2 cursorOffset = renderer.measureText(textBeforeCursor, theme.fontSize);
            float cursorX = textArea.x + cursorOffset.x;
            renderer.drawRect({ cursorX, textArea.y + 2, 1, textArea.h - 4 }, theme.text);
        }

        renderer.popClip();
    }

    bool TextField::handleMouse(const MouseEvent& event) {
        bool inside = bounds.contains(event.x, event.y);

        if (event.pressed && event.button == MouseButton::Left) {
            bool wasFocused = focused_;
            focused_ = inside;

            if (focused_ && !wasFocused) {
                cursorPos_ = text.length();
                cursorVisible_ = true;
            }
            return inside;
        }

        return inside && focused_;
    }

    bool TextField::handleKey(const KeyEvent& event) {
        if (!focused_ || !event.pressed) return false;

        // Handle special keys
        if (event.key == GLFW_KEY_BACKSPACE && cursorPos_ > 0) {
            text.erase(cursorPos_ - 1, 1);
            cursorPos_--;
            if (onChange) onChange(text);
            return true;
        }

        if (event.key == GLFW_KEY_DELETE && cursorPos_ < text.length()) {
            text.erase(cursorPos_, 1);
            if (onChange) onChange(text);
            return true;
        }

        if (event.key == GLFW_KEY_LEFT && cursorPos_ > 0) {
            cursorPos_--;
            return true;
        }

        if (event.key == GLFW_KEY_RIGHT && cursorPos_ < text.length()) {
            cursorPos_++;
            return true;
        }

        if (event.key == GLFW_KEY_HOME) {
            cursorPos_ = 0;
            return true;
        }

        if (event.key == GLFW_KEY_END) {
            cursorPos_ = text.length();
            return true;
        }

        if (event.key == GLFW_KEY_ENTER) {
            if (onSubmit) onSubmit(text);
            return true;
        }

        // Handle printable characters (ASCII range for simplicity)
        if (event.key >= 32 && event.key <= 126 && text.length() < static_cast<size_t>(maxLength)) {
            char c = static_cast<char>(event.key);
            if (!event.shift && c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';  // Convert to lowercase
            }
            text.insert(cursorPos_, 1, c);
            cursorPos_++;
            if (onChange) onChange(text);
            return true;
        }

        return false;
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
                contentHeight_ += theme.buttonHeight + theme.spacing;
            }
        }

        maxScroll_ = std::max(0.0f, contentHeight_ - bounds.h);
        scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll_);

        // Layout children with scroll offset
        float y = bounds.y - scrollOffset + theme.padding;
        for (auto& child : children_) {
            if (!child->visible) continue;

            Rect childBounds = {
                bounds.x + theme.padding,
                y,
                bounds.w - theme.padding * 2 - (showScrollbar && maxScroll_ > 0 ? 12.0f : 0),
                theme.buttonHeight
            };
            child->layout(childBounds);
            y += childBounds.h + theme.spacing;
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

            Color thumbColor = scrollbarDragging_ ? theme.accent : theme.buttonHover;
            renderer.drawRoundedRect(thumbBounds_, thumbColor, 4.0f);
        }
    }

    bool ScrollArea::handleMouse(const MouseEvent& event) {
        // Handle scroll wheel
        if (bounds.contains(event.x, event.y) && event.scroll != 0) {
            scrollOffset -= event.scroll * 30.0f;
            scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll_);
            layout(bounds);  // Relayout with new offset
            return true;
        }

        // Handle scrollbar dragging
        if (showScrollbar && maxScroll_ > 0) {
            if (thumbBounds_.contains(event.x, event.y) && event.pressed) {
                scrollbarDragging_ = true;
                dragStartOffset_ = event.y - thumbBounds_.y;
                return true;
            }

            if (scrollbarDragging_) {
                if (event.released) {
                    scrollbarDragging_ = false;
                }
                else {
                    float newThumbY = event.y - dragStartOffset_;
                    float thumbRange = bounds.h - thumbBounds_.h;
                    float t = (newThumbY - bounds.y) / thumbRange;
                    scrollOffset = t * maxScroll_;
                    scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll_);
                    layout(bounds);
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

    // ============================================================================
    // PROPERTY ROW
    // ============================================================================

    PropertyRow::PropertyRow(const std::string& label, std::unique_ptr<Widget> control)
        : label(label), control_(std::move(control)) {
    }

    void PropertyRow::layout(const Rect& available) {
        bounds = available;

        labelBounds_ = { bounds.x, bounds.y, labelWidth, bounds.h };
        controlBounds_ = { bounds.x + labelWidth, bounds.y, bounds.w - labelWidth, bounds.h };

        if (control_) {
            control_->layout(controlBounds_);
        }
    }

    void PropertyRow::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Draw label
        float labelY = labelBounds_.y + (labelBounds_.h - theme.fontSize) / 2;
        renderer.drawText(label, labelBounds_.x, labelY, theme.text, theme.fontSize);

        // Draw control
        if (control_) {
            control_->draw(renderer);
        }
    }

    bool PropertyRow::handleMouse(const MouseEvent& event) {
        if (control_ && controlBounds_.contains(event.x, event.y)) {
            return control_->handleMouse(event);
        }
        return false;
    }

    bool PropertyRow::handleKey(const KeyEvent& event) {
        if (control_) {
            return control_->handleKey(event);
        }
        return false;
    }

    // ============================================================================
    // COLLAPSIBLE SECTION
    // ============================================================================

    CollapsibleSection::CollapsibleSection(const std::string& title) : title(title) {}

    void CollapsibleSection::layout(const Rect& available) {
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

            // Layout children
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

    void CollapsibleSection::draw(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Header
        Color headerBg = headerHovered_ ? theme.panelHeaderHover : theme.panelHeader;
        renderer.drawRect(headerBounds_, headerBg);

        // Collapse indicator
        std::string indicator = collapsed ? "+" : "-";
        renderer.drawText(indicator, headerBounds_.x + theme.padding,
            headerBounds_.y + (headerBounds_.h - theme.fontSize) / 2,
            theme.textDim, theme.fontSize);

        // Title
        renderer.drawText(title, headerBounds_.x + theme.padding + 16,
            headerBounds_.y + (headerBounds_.h - theme.fontSize) / 2,
            theme.text, theme.fontSize);

        // Draw children if not collapsed
        if (!collapsed) {
            for (auto& child : children_) {
                if (child->visible) {
                    child->draw(renderer);
                }
            }
        }
    }

    bool CollapsibleSection::handleMouse(const MouseEvent& event) {
        headerHovered_ = headerBounds_.contains(event.x, event.y);

        if (headerHovered_ && event.pressed && event.button == MouseButton::Left) {
            collapsed = !collapsed;
            return true;
        }

        if (!collapsed) {
            return Widget::handleMouse(event);
        }

        return headerHovered_;
    }

} // namespace libre::ui