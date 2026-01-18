// src/ui/UI.cpp
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
        (void)id;
        return nullptr;
    }

    void UIManager::onMouseMove(float x, float y) {
        mouseX_ = x;
        mouseY_ = y;

        MouseEvent event;
        event.x = x;
        event.y = y;

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

        for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
            if ((*it)->handleMouse(event)) return;
        }

        if (menuBar_ && menuBar_->handleMouse(event)) return;

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

        if (menuBar_) {
            menuBar_->layout({ 0, 0, screenWidth, theme.panelHeaderHeight });
        }

        float y = menuBarHeight;
        float panelWidth = 250.0f;

        for (auto& widget : widgets_) {
            float panelHeight = 200.0f;
            widget->layout({ 0, y, panelWidth, panelHeight });
            y += panelHeight;
        }

        for (auto& window : windows_) {
            window->layout(window->bounds);
        }
    }

    void UIManager::render(VkCommandBuffer cmd) {
        renderer_.begin(screenWidth_, screenHeight_);

        for (auto& widget : widgets_) {
            if (widget->visible) {
                widget->draw(renderer_);
            }
        }

        if (menuBar_) {
            menuBar_->draw(renderer_);
        }

        for (auto& window : windows_) {
            if (window->isOpen) {
                window->draw(renderer_);
            }
        }

        renderer_.end(cmd);
    }

} // namespace libre::ui