// LibreUI/src/Widgets/Widget.cpp
// Base widget implementation

#include <LibreUI/Widget.h>
#include <algorithm>

namespace LibreUI {

    // ========================================================================
    // LAYOUT
    // ========================================================================

    void Widget::layout(const Rect& available) {
        bounds = available;

        // Layout children with same bounds by default
        // Derived classes override for specific layouts
        for (auto& child : children_) {
            child->layout(available);
        }
    }

    // ========================================================================
    // DRAWING
    // ========================================================================

    void Widget::draw(Renderer& renderer) {
        // Base widget just draws children
        for (auto& child : children_) {
            if (child->visible) {
                child->draw(renderer);
            }
        }
    }

    // ========================================================================
    // INPUT HANDLING
    // ========================================================================

    bool Widget::handleMouse(const MouseEvent& event) {
        // Process children in reverse order (top-most first)
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            if ((*it)->visible && (*it)->handleMouse(event)) {
                return true;
            }
        }
        return false;
    }

    bool Widget::handleKey(const KeyEvent& event) {
        // Pass to children
        for (auto& child : children_) {
            if (child->handleKey(event)) {
                return true;
            }
        }
        return false;
    }

    // ========================================================================
    // CHILD MANAGEMENT
    // ========================================================================

    void Widget::addChild(std::unique_ptr<Widget> child) {
        child->parent_ = this;
        children_.push_back(std::move(child));
    }

    void Widget::removeChild(Widget* child) {
        auto it = std::find_if(children_.begin(), children_.end(),
            [child](const std::unique_ptr<Widget>& ptr) {
                return ptr.get() == child;
            });

        if (it != children_.end()) {
            (*it)->parent_ = nullptr;
            children_.erase(it);
        }
    }

    void Widget::clearChildren() {
        for (auto& child : children_) {
            child->parent_ = nullptr;
        }
        children_.clear();
    }

} // namespace LibreUI