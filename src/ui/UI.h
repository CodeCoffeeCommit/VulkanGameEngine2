// src/ui/UI.h
#pragma once

#include "Core.h"
#include "Theme.h"
#include "UIRenderer.h"
#include "Widgets.h"
#include <memory>
#include <vector>

class VulkanContext;

namespace libre::ui {

    class UIManager {
    public:
        void init(VulkanContext* context, VkRenderPass renderPass);
        void cleanup();

        // Add root-level widgets
        void addWidget(std::unique_ptr<Widget> widget);

        // Add floating windows (drawn on top)
        void addWindow(std::unique_ptr<Window> window);

        // Set the menu bar
        void setMenuBar(std::unique_ptr<MenuBar> menuBar);

        // Find widgets by walking the tree (simple linear search)
        Widget* findWidget(const std::string& id);

        // Input handling
        void onMouseMove(float x, float y);
        void onMouseButton(MouseButton button, bool pressed);
        void onMouseScroll(float delta);
        void onKey(int key, bool pressed, bool shift, bool ctrl, bool alt);

        // Update layout when screen size changes
        void layout(float screenWidth, float screenHeight);

        // Render all UI
        void render(VkCommandBuffer cmd);

        // Access
        UIRenderer& getRenderer() { return renderer_; }
        float getScreenWidth() const { return screenWidth_; }
        float getScreenHeight() const { return screenHeight_; }

    private:
        UIRenderer renderer_;

        // Menu bar (top of screen)
        std::unique_ptr<MenuBar> menuBar_;

        // Root widgets (panels docked to sides, etc.)
        std::vector<std::unique_ptr<Widget>> widgets_;

        // Floating windows (drawn on top)
        std::vector<std::unique_ptr<Window>> windows_;

        // Mouse state
        float mouseX_ = 0;
        float mouseY_ = 0;

        float screenWidth_ = 0;
        float screenHeight_ = 0;
    };

} // namespace libre::ui