// LibreUI/src/Widgets/Label.cpp
// Simple text label implementation

#include <LibreUI/Widgets/Label.h>
#include <LibreUI/Renderer.h>

namespace LibreUI {

    Label::Label(const std::string& text) : text(text) {
        color = GetTheme().text;
    }

    void Label::draw(Renderer& renderer) {
        renderer.drawText(text, bounds.x, bounds.y, color, fontSize);
    }

} // namespace LibreUI