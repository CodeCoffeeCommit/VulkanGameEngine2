#include "UI.h"
#include <iostream>

namespace libre::ui {

    void UIManager::init(VulkanContext* context, VkRenderPass renderPass) {
        renderer_.init(context, renderPass);
    }

    void UIManager::cleanup() {
        windows_.clear();
        widgets_.clear();
        menuBar_.reset();
        renderer_.cleanup();
    }

    void UIManager::addWidget(std::unique_ptr<Widget> widget) {
        widgets_.push_back(std::move(widget));
    }

    void UIManager::addWindow(std::unique_ptr<Window> window) {
        windows_.push_back(std::move(window));
    }

    void UIManager::setMenuBar(std::unique_ptr<MenuBar> menuBar) {
        menuBar_ = std::move(menuBar);
    }

    Widget* UIManager::findWidget(const std::string& id) {
        // Simple search - extend later if needed
        (void)id;
        return nullptr;
    }

    void UIManager::onMouseMove(float x, float y) {
        mouseX_ = x;
        mouseY_ = y;

        // Create move event
        MouseEvent event;
        event.x = x;
        event.y = y;

        // Update hover states (windows first, they're on top)
        for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
            (*it)->handleMouse(event);
        }

        if (menuBar_) {
            menuBar_->handleMouse(event);
        }

        for (auto& widget : widgets_) {
            widget->handleMouse(event);
        }
    }

    void UIManager::onMouseButton(MouseButton button, bool pressed) {
        MouseEvent event;
        event.x = mouseX_;
        event.y = mouseY_;
        event.button = button;
        event.pressed = pressed;
        event.released = !pressed;

        // Windows first (topmost)
        for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
            if ((*it)->handleMouse(event)) return;
        }

        // Menu bar
        if (menuBar_ && menuBar_->handleMouse(event)) return;

        // Root widgets
        for (auto& widget : widgets_) {
            if (widget->handleMouse(event)) return;
        }
    }

    void UIManager::onMouseScroll(float delta) {
        MouseEvent event;
        event.x = mouseX_;
        event.y = mouseY_;
        event.scroll = delta;

        for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
            if ((*it)->handleMouse(event)) return;
        }

        for (auto& widget : widgets_) {
            if (widget->handleMouse(event)) return;
        }
    }

    void UIManager::onKey(int key, bool pressed, bool shift, bool ctrl, bool alt) {
        KeyEvent event;
        event.key = key;
        event.pressed = pressed;
        event.shift = shift;
        event.ctrl = ctrl;
        event.alt = alt;

        for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
            if ((*it)->handleKey(event)) return;
        }

        for (auto& widget : widgets_) {
            if (widget->handleKey(event)) return;
        }
    }

    void UIManager::layout(float screenWidth, float screenHeight) {
        screenWidth_ = screenWidth;
        screenHeight_ = screenHeight;

        auto& theme = GetTheme();
        float menuBarHeight = menuBar_ ? theme.panelHeaderHeight : 0;

        // Menu bar at top
        if (menuBar_) {
            menuBar_->layout({ 0, 0, screenWidth, theme.panelHeaderHeight });
        }

        // Layout root widgets (for now, just stack them on the left side)
        float y = menuBarHeight;
        float panelWidth = 250.0f;  // Fixed width for side panels

        for (auto& widget : widgets_) {
            float panelHeight = 200.0f;  // Default height
            widget->layout({ 0, y, panelWidth, panelHeight });
            y += panelHeight;
        }

        // Windows keep their own position, just recalculate internal layout
        for (auto& window : windows_) {
            window->layout(window->bounds);
        }
    }


    // In src/ui/UI.cpp - Replace the render() function with this:

    void UIManager::render(VkCommandBuffer cmd) {
        renderer_.begin(screenWidth_, screenHeight_);

        // ============================================
        // TEST: Draw a bright red square in the center
        // ============================================
        float testSize = 200.0f;
        float centerX = (screenWidth_ - testSize) / 2.0f;
        float centerY = (screenHeight_ - testSize) / 2.0f;

        // Bright red square
        renderer_.drawRect(
            { centerX, centerY, testSize, testSize },
            Color(1.0f, 0.0f, 0.0f, 1.0f)  // Red
        );

        // Smaller green square inside
        renderer_.drawRect(
            { centerX + 20, centerY + 20, testSize - 40, testSize - 40 },
            Color(0.0f, 1.0f, 0.0f, 1.0f)  // Green
        );

        // Even smaller blue square
        renderer_.drawRect(
            { centerX + 40, centerY + 40, testSize - 80, testSize - 80 },
            Color(0.0f, 0.0f, 1.0f, 1.0f)  // Blue
        );

        // Debug output
        std::cout << "[TEST] Drew 3 test squares. Vertex count: "
            << "screen=" << screenWidth_ << "x" << screenHeight_ << std::endl;
        // ============================================
        // END TEST
        // ============================================

        // Draw root widgets
        for (auto& widget : widgets_) {
            if (widget->visible) {
                widget->draw(renderer_);
            }
        }

        // Draw menu bar
        if (menuBar_) {
            menuBar_->draw(renderer_);
        }

        // Draw windows on top
        for (auto& window : windows_) {
            if (window->isOpen) {
                window->draw(renderer_);
            }
        }

        renderer_.end(cmd);
    }
    /*void UIManager::render(VkCommandBuffer cmd) {
        std::cout << "[DEBUG] UIManager::render - screenWidth=" << screenWidth_
            << " screenHeight=" << screenHeight_
            << " menuBar=" << (menuBar_ ? "yes" : "no") << std::endl;  // ADD THIS


        renderer_.begin(screenWidth_, screenHeight_);

        // Draw root widgets
        for (auto& widget : widgets_) {
            if (widget->visible) {
                widget->draw(renderer_);
            }
        }

        // Draw menu bar
        if (menuBar_) {
            menuBar_->draw(renderer_);
        }

        // Draw windows on top
        for (auto& window : windows_) {
            if (window->isOpen) {
                window->draw(renderer_);
            }
        }

        renderer_.end(cmd);
    }
    */
} // namespace libre::ui