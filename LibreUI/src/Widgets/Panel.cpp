// LibreUI/src/Widgets/Panel.cpp
// Collapsible panel implementation

#include <LibreUI/Widgets/Panel.h>
#include <LibreUI/Renderer.h>

namespace LibreUI {

    Panel::Panel(const std::string& title) : title(title) {}

    void Panel::layout(const Rect& available) {
        bounds = available;
        auto& theme = GetTheme();

        // Header is always visible
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

    void Panel::draw(Renderer& renderer) {
        auto& theme = GetTheme();

        // Header background
        Color headerColor = hovered ? theme.panelHeaderHover : theme.panelHeader;
        renderer.drawRoundedRect(headerBounds_, headerColor, theme.cornerRadius());

        // Collapse indicator
        std::string indicator = collapsed ? ">" : "v";
        renderer.drawText(indicator, headerBounds_.x + 8, headerBounds_.y + 6,
            theme.text, theme.fontSize());

        // Title
        renderer.drawText(title, headerBounds_.x + 24, headerBounds_.y + 6,
            theme.text, theme.fontSize());

        // Content area (if not collapsed)
        if (!collapsed && contentBounds_.h > 0) {
            renderer.drawRect(contentBounds_, theme.background);

            // Draw children with clipping
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

} // namespace LibreUI