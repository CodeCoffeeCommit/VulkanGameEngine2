// LibreUI/src/Widgets/Window.cpp
// Floating window implementation

#include <LibreUI/Widgets/Window.h>
#include <LibreUI/Renderer.h>

namespace LibreUI {

    Window::Window(const std::string& title) : title(title) {}

    void Window::layout(const Rect& available) {
        // Window uses its own bounds, not available rect
        // (windows are positioned absolutely)
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

    void Window::draw(Renderer& renderer) {
        if (!isOpen) return;

        auto& theme = GetTheme();

        // Shadow
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
            Color closeColor = closeHovered_ ? theme.accent : theme.textDim;
            renderer.drawText("X", closeButtonBounds_.x + 4, closeButtonBounds_.y + 2,
                closeColor, theme.fontSize());
        }

        // Content area
        renderer.pushClip(contentBounds_);
        Widget::draw(renderer);
        renderer.popClip();
    }

    bool Window::handleMouse(const MouseEvent& event) {
        if (!isOpen) return false;

        // Update close button hover
        closeHovered_ = closable && closeButtonBounds_.contains(event.x, event.y);

        // Close button click
        if (closeHovered_ && event.pressed && event.button == MouseButton::Left) {
            isOpen = false;
            if (onClose) {
                onClose();
            }
            return true;
        }

        // Dragging
        if (draggable && titleBarBounds_.contains(event.x, event.y)) {
            if (event.pressed && event.button == MouseButton::Left && !closeHovered_) {
                dragging_ = true;
                dragOffsetX_ = event.x - bounds.x;
                dragOffsetY_ = event.y - bounds.y;
                return true;
            }
        }

        if (dragging_) {
            if (event.released && event.button == MouseButton::Left) {
                dragging_ = false;
            }
            else {
                // Update position
                bounds.x = event.x - dragOffsetX_;
                bounds.y = event.y - dragOffsetY_;
            }
            return true;
        }

        // Pass to children
        if (contentBounds_.contains(event.x, event.y)) {
            return Widget::handleMouse(event);
        }

        return bounds.contains(event.x, event.y);
    }

} // namespace LibreUI