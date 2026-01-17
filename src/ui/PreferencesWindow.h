#pragma once

#include "Widgets.h"
#include <vector>
#include <string>

namespace libre::ui {

    // ============================================================================
    // PREFERENCES TAB CONTENT
    // ============================================================================

    enum class PreferencesTab {
        General,
        Interface,
        Viewport,
        Credits
    };

    // ============================================================================
    // PREFERENCES WINDOW
    // A specialized floating window for application preferences
    // ============================================================================

    class PreferencesWindow : public Window {
    public:
        PreferencesWindow();

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

    private:
        void drawSidebar(UIRenderer& renderer);
        void drawContent(UIRenderer& renderer);

        void drawGeneralTab(UIRenderer& renderer);
        void drawInterfaceTab(UIRenderer& renderer);
        void drawViewportTab(UIRenderer& renderer);
        void drawCreditsTab(UIRenderer& renderer);

        PreferencesTab currentTab_ = PreferencesTab::Credits;  // Default to Credits for demo

        // Layout regions
        Rect sidebarBounds_;
        Rect contentBounds_;

        // Tab buttons
        struct TabButton {
            std::string label;
            PreferencesTab tab;
            Rect bounds;
            bool hovered = false;
        };
        std::vector<TabButton> tabButtons_;

        // Sidebar width
        static constexpr float SIDEBAR_WIDTH = 120.0f;
    };

} // namespace libre::ui