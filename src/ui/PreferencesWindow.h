// src/ui/PreferencesWindow.h
#pragma once

#include "Widgets.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace libre::ui {

    // ============================================================================
    // PREFERENCES TAB CONTENT
    // ============================================================================

    enum class PreferencesTab {
        General,
        Interface,
        Viewport,
        Input,
        Performance,
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
        bool handleKey(const KeyEvent& event) override;

        // ========================================================================
        // SETTINGS DATA
        // Access current settings (for Application to read/write)
        // ========================================================================

        struct Settings {
            // General
            std::string projectPath = "";
            bool autoSave = true;
            int autoSaveInterval = 5;  // minutes

            // Interface
            float uiScale = 1.0f;
            bool showTooltips = true;
            bool animateUI = true;
            int themeIndex = 0;  // 0=Dark, 1=Light, 2=Custom

            // Viewport
            float fov = 60.0f;
            float nearClip = 0.1f;
            float farClip = 10000.0f;
            bool vsync = true;
            bool showGrid = true;
            bool showAxes = true;
            Color gridColor{ 0.3f, 0.3f, 0.3f, 1.0f };
            Color backgroundColor{ 0.18f, 0.18f, 0.18f, 1.0f };

            // Input
            float mouseSensitivity = 1.0f;
            float zoomSpeed = 1.0f;
            float panSpeed = 1.0f;
            bool invertY = false;
            bool invertZoom = false;

            // Performance
            int maxUndoSteps = 50;
            bool useGPUCompute = true;
            int textureQuality = 2;  // 0=Low, 1=Medium, 2=High
        };

        Settings& getSettings() { return settings_; }
        const Settings& getSettings() const { return settings_; }

    private:
        // ========================================================================
        // DRAWING METHODS
        // ========================================================================

        void drawSidebar(UIRenderer& renderer);
        void drawContent(UIRenderer& renderer);

        void drawGeneralTab(UIRenderer& renderer);
        void drawInterfaceTab(UIRenderer& renderer);
        void drawViewportTab(UIRenderer& renderer);
        void drawInputTab(UIRenderer& renderer);
        void drawPerformanceTab(UIRenderer& renderer);
        void drawCreditsTab(UIRenderer& renderer);

        // ========================================================================
        // WIDGET DRAWING HELPERS
        // ========================================================================

        void drawSectionHeader(UIRenderer& renderer, float& y, const std::string& title);
        void drawSeparator(UIRenderer& renderer, float& y);
        Rect drawPropertyRow(UIRenderer& renderer, float& y, const std::string& label);
        Rect drawCheckbox(UIRenderer& renderer, const Rect& bounds, bool checked, bool hovered = false);
        Rect drawSlider(UIRenderer& renderer, const Rect& bounds, float value, float minVal, float maxVal,
            const std::string& valueFormat = "%.1f", bool hovered = false);
        Rect drawTextField(UIRenderer& renderer, const Rect& bounds, const std::string& text,
            const std::string& placeholder = "", bool focused = false);
        Rect drawNumericField(UIRenderer& renderer, const Rect& bounds, float value,
            const std::string& format = "%.0f");
        Rect drawDropdown(UIRenderer& renderer, const Rect& bounds, const std::vector<std::string>& items,
            int selectedIndex, bool open = false);

        // ========================================================================
        // INPUT HANDLING HELPERS
        // ========================================================================

        void handleCheckboxClick(int widgetId);

        // ========================================================================
        // STATE
        // ========================================================================

        PreferencesTab currentTab_ = PreferencesTab::General;

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

        // Settings data
        Settings settings_;

        // Scroll state per tab
        std::unordered_map<PreferencesTab, float> scrollOffsets_;
        float currentScrollOffset_ = 0.0f;
        float contentHeight_ = 0.0f;

        // Scrollbar interaction
        bool scrollbarDragging_ = false;
        float scrollDragStartY_ = 0.0f;
        float scrollDragStartOffset_ = 0.0f;
        Rect scrollbarThumbBounds_;

        // Widget interaction tracking
        struct WidgetHitArea {
            Rect bounds;
            int id;
            enum class Type { Checkbox, Slider, TextField, Button, Dropdown } type;
        };
        std::vector<WidgetHitArea> hitAreas_;
        int hoveredWidgetId_ = -1;
        int activeWidgetId_ = -1;
        float sliderDragStartValue_ = 0.0f;

        // Constants
        static constexpr float SIDEBAR_WIDTH = 130.0f;
        static constexpr float ROW_HEIGHT = 28.0f;
        static constexpr float LABEL_WIDTH = 140.0f;
        static constexpr float CHECKBOX_SIZE = 16.0f;
        static constexpr float SLIDER_HANDLE_WIDTH = 12.0f;
        static constexpr float SCROLLBAR_WIDTH = 10.0f;
    };

} // namespace libre::ui