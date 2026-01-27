// LibreUI/src/Widgets/Button.cpp
// Clickable button implementation

#include <LibreUI/Widgets/Button.h>
#include <LibreUI/Renderer.h>

namespace LibreUI {

    Button::Button(const std::string& text) : text(text) {}

    void Button::draw(Renderer& renderer) {
        auto& theme = GetTheme();

        // Determine background color based on state
        Color bgColor = theme.buttonBackground;
        if (pressed) {
            bgColor = theme.buttonPressed;
        }
        else if (hovered) {
            bgColor = theme.buttonHover;
        }

        // Draw button background
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

} // namespace LibreUI