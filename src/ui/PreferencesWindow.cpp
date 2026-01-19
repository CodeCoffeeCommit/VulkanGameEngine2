// src/ui/PreferencesWindow.cpp
// Updated to use scaled theme getters instead of direct member access

#include "PreferencesWindow.h"
#include <cstdio>
#include <algorithm>
#include <iostream>

namespace libre::ui {

    // Helper function for clamping
    template<typename T>
    T clampVal(T val, T minV, T maxV) {
        return std::max(minV, std::min(maxV, val));
    }

    // ============================================================================
    // CONSTRUCTION
    // ============================================================================

    PreferencesWindow::PreferencesWindow() : Window("Preferences") {
        bounds = { 100, 100, 600, 500 };
        closable = true;
        draggable = true;
        isOpen = false;

        // Initialize tab buttons
        tabButtons_ = {
            { "General",     PreferencesTab::General,     {}, false },
            { "Interface",   PreferencesTab::Interface,   {}, false },
            { "Viewport",    PreferencesTab::Viewport,    {}, false },
            { "Input",       PreferencesTab::Input,       {}, false },
            { "Performance", PreferencesTab::Performance, {}, false },
            { "Credits",     PreferencesTab::Credits,     {}, false }
        };

        // Initialize scroll offsets for each tab
        for (int i = 0; i < 6; i++) {
            scrollOffsets_[static_cast<PreferencesTab>(i)] = 0.0f;
        }
    }

    // ============================================================================
    // LAYOUT
    // ============================================================================

    void PreferencesWindow::layout(const Rect& available) {
        Window::layout(available);

        auto& theme = GetTheme();

        // Sidebar on the left
        sidebarBounds_ = {
            contentBounds_.x,
            contentBounds_.y,
            SIDEBAR_WIDTH,
            contentBounds_.h
        };

        // Content area takes the rest
        contentBounds_ = {
            contentBounds_.x + SIDEBAR_WIDTH,
            contentBounds_.y,
            bounds.w - SIDEBAR_WIDTH,
            contentBounds_.h
        };

        // Layout tab buttons
        float y = sidebarBounds_.y + theme.padding();
        for (auto& btn : tabButtons_) {
            btn.bounds = {
                sidebarBounds_.x + theme.padding(),
                y,
                SIDEBAR_WIDTH - theme.padding() * 2,
                ROW_HEIGHT
            };
            y += ROW_HEIGHT + 2;
        }
    }

    // ============================================================================
    // DRAWING
    // ============================================================================

    void PreferencesWindow::draw(UIRenderer& renderer) {
        if (!isOpen) return;

        Window::draw(renderer);

        hitAreas_.clear();

        drawSidebar(renderer);
        drawContent(renderer);
    }

    // ============================================================================
    // SIDEBAR
    // ============================================================================

    void PreferencesWindow::drawSidebar(UIRenderer& renderer) {
        auto& theme = GetTheme();

        // Sidebar background
        renderer.drawRect(sidebarBounds_, theme.backgroundDark);

        // Vertical separator line
        renderer.drawRect({
            sidebarBounds_.right() - 1,
            sidebarBounds_.y,
            1,
            sidebarBounds_.h
            }, theme.border);

        // Tab buttons
        for (auto& btn : tabButtons_) {
            Color bgColor = theme.backgroundDark;
            if (btn.tab == currentTab_) {
                bgColor = theme.accent;
            }
            else if (btn.hovered) {
                bgColor = theme.buttonHover;
            }

            renderer.drawRoundedRect(btn.bounds, bgColor, theme.cornerRadius());

            Vec2 textSize = renderer.measureText(btn.label, theme.fontSize());
            float textX = btn.bounds.x + (btn.bounds.w - textSize.x) / 2;
            float textY = btn.bounds.y + (btn.bounds.h - textSize.y) / 2;
            renderer.drawText(btn.label, textX, textY, theme.text, theme.fontSize());
        }
    }

    // ============================================================================
    // CONTENT AREA
    // ============================================================================

    void PreferencesWindow::drawContent(UIRenderer& renderer) {
        auto& theme = GetTheme();

        renderer.drawRect(contentBounds_, theme.background);
        renderer.pushClip(contentBounds_);

        currentScrollOffset_ = scrollOffsets_[currentTab_];

        switch (currentTab_) {
        case PreferencesTab::General:     drawGeneralTab(renderer); break;
        case PreferencesTab::Interface:   drawInterfaceTab(renderer); break;
        case PreferencesTab::Viewport:    drawViewportTab(renderer); break;
        case PreferencesTab::Input:       drawInputTab(renderer); break;
        case PreferencesTab::Performance: drawPerformanceTab(renderer); break;
        case PreferencesTab::Credits:     drawCreditsTab(renderer); break;
        }

        float maxScroll = contentHeight_ - contentBounds_.h;
        if (maxScroll < 0) maxScroll = 0;

        // Draw scrollbar if needed
        if (maxScroll > 0) {
            Rect scrollTrack = {
                contentBounds_.right() - SCROLLBAR_WIDTH - 2,
                contentBounds_.y + 2,
                SCROLLBAR_WIDTH,
                contentBounds_.h - 4
            };
            renderer.drawRoundedRect(scrollTrack, theme.backgroundDark, SCROLLBAR_WIDTH / 2);

            float thumbHeight = (contentBounds_.h / contentHeight_) * scrollTrack.h;
            if (thumbHeight < 30.0f) thumbHeight = 30.0f;
            float thumbY = scrollTrack.y + (currentScrollOffset_ / maxScroll) * (scrollTrack.h - thumbHeight);

            scrollbarThumbBounds_ = { scrollTrack.x, thumbY, scrollTrack.w, thumbHeight };

            Color thumbColor = scrollbarDragging_ ? theme.accent : theme.scrollbarThumb;
            renderer.drawRoundedRect(scrollbarThumbBounds_, thumbColor, SCROLLBAR_WIDTH / 2);
        }

        renderer.popClip();
    }

    // ============================================================================
    // DRAWING HELPERS
    // ============================================================================

    void PreferencesWindow::drawSectionHeader(UIRenderer& renderer, float& y, const std::string& title) {
        auto& theme = GetTheme();
        float scrollY = y - currentScrollOffset_;

        if (scrollY + 30 > contentBounds_.y && scrollY < contentBounds_.bottom()) {
            renderer.drawText(title, contentBounds_.x + theme.padding(), scrollY,
                theme.accent, theme.fontSize() + 2);
        }
        y += 30;
    }

    void PreferencesWindow::drawSeparator(UIRenderer& renderer, float& y) {
        auto& theme = GetTheme();
        y += 10;
        float scrollY = y - currentScrollOffset_;

        if (scrollY > contentBounds_.y && scrollY < contentBounds_.bottom()) {
            renderer.drawRect({
                contentBounds_.x + theme.padding(),
                scrollY,
                contentBounds_.w - theme.padding() * 2 - SCROLLBAR_WIDTH - 4,
                1
                }, theme.border);
        }
        y += 15;
    }

    Rect PreferencesWindow::drawPropertyRow(UIRenderer& renderer, float& y, const std::string& label) {
        auto& theme = GetTheme();
        float scrollY = y - currentScrollOffset_;
        float x = contentBounds_.x + theme.padding();

        Rect controlBounds = {
            x + LABEL_WIDTH,
            scrollY,
            contentBounds_.w - LABEL_WIDTH - theme.padding() * 2 - SCROLLBAR_WIDTH - 4,
            ROW_HEIGHT
        };

        if (scrollY + ROW_HEIGHT > contentBounds_.y && scrollY < contentBounds_.bottom()) {
            renderer.drawText(label, x, scrollY + (ROW_HEIGHT - theme.fontSize()) / 2,
                theme.text, theme.fontSize());
        }

        y += ROW_HEIGHT + theme.spacing();
        return controlBounds;
    }

    Rect PreferencesWindow::drawCheckbox(UIRenderer& renderer, const Rect& bounds, bool checked, bool hovered) {
        auto& theme = GetTheme();

        Rect box = {
            bounds.x,
            bounds.y + (bounds.h - CHECKBOX_SIZE) / 2,
            CHECKBOX_SIZE,
            CHECKBOX_SIZE
        };

        if (box.bottom() > contentBounds_.y && box.y < contentBounds_.bottom()) {
            Color boxBg = hovered ? theme.buttonHover : theme.buttonBackground;
            renderer.drawRoundedRect(box, boxBg, 3.0f);
            renderer.drawRectOutline(box, theme.border);

            if (checked) {
                Rect inner = box.shrink(4.0f);
                renderer.drawRoundedRect(inner, theme.accent, 2.0f);
            }
        }

        return box;
    }

    Rect PreferencesWindow::drawSlider(UIRenderer& renderer, const Rect& bounds, float value,
        float minVal, float maxVal, const std::string& valueFormat, bool hovered) {
        auto& theme = GetTheme();

        float valueDisplayWidth = 50.0f;
        float trackWidth = bounds.w - valueDisplayWidth - theme.padding();

        float trackH = 4.0f;
        Rect track = {
            bounds.x,
            bounds.y + (bounds.h - trackH) / 2,
            trackWidth,
            trackH
        };

        if (track.bottom() > contentBounds_.y && track.y < contentBounds_.bottom()) {
            renderer.drawRoundedRect(track, theme.backgroundDark, trackH / 2);

            float t = (value - minVal) / (maxVal - minVal);
            t = clampVal(t, 0.0f, 1.0f);

            Rect filled = { track.x, track.y, track.w * t, track.h };
            renderer.drawRoundedRect(filled, theme.accent, trackH / 2);

            float handleX = track.x + track.w * t - SLIDER_HANDLE_WIDTH / 2;
            Rect handle = { handleX, bounds.y + 4, SLIDER_HANDLE_WIDTH, bounds.h - 8 };

            Color handleColor = hovered ? theme.accentHover : theme.buttonBackground;
            renderer.drawRoundedRect(handle, handleColor, 4.0f);
            renderer.drawRectOutline(handle, theme.border);

            char buf[32];
            snprintf(buf, sizeof(buf), valueFormat.c_str(), value);
            renderer.drawText(buf, track.right() + theme.padding(),
                bounds.y + (bounds.h - theme.fontSize()) / 2,
                theme.textDim, theme.fontSize());
        }

        return track;
    }

    Rect PreferencesWindow::drawTextField(UIRenderer& renderer, const Rect& bounds,
        const std::string& text, const std::string& placeholder, bool focused) {
        auto& theme = GetTheme();

        if (bounds.bottom() > contentBounds_.y && bounds.y < contentBounds_.bottom()) {
            Color bgColor = focused ? theme.backgroundLight : theme.buttonBackground;
            renderer.drawRoundedRect(bounds, bgColor, theme.cornerRadius());

            Color borderColor = focused ? theme.accent : theme.border;
            renderer.drawRectOutline(bounds, borderColor);

            std::string displayText = text.empty() ? placeholder : text;
            Color textColor = text.empty() ? theme.textDim : theme.text;

            renderer.drawText(displayText, bounds.x + theme.padding(),
                bounds.y + (bounds.h - theme.fontSize()) / 2,
                textColor, theme.fontSize());
        }

        return bounds;
    }

    Rect PreferencesWindow::drawNumericField(UIRenderer& renderer, const Rect& bounds,
        float value, const std::string& format) {
        auto& theme = GetTheme();

        if (bounds.bottom() > contentBounds_.y && bounds.y < contentBounds_.bottom()) {
            Rect fieldBounds = { bounds.x, bounds.y, bounds.w, bounds.h };
            renderer.drawRoundedRect(fieldBounds, theme.buttonBackground, theme.cornerRadius());
            renderer.drawRectOutline(fieldBounds, theme.border);

            char buf[32];
            snprintf(buf, sizeof(buf), format.c_str(), value);
            renderer.drawText(buf, bounds.x + theme.padding(),
                bounds.y + (bounds.h - theme.fontSize()) / 2,
                theme.text, theme.fontSize());
        }

        return bounds;
    }

    Rect PreferencesWindow::drawDropdown(UIRenderer& renderer, const Rect& bounds,
        const std::vector<std::string>& items, int selectedIndex, bool open) {
        auto& theme = GetTheme();

        if (bounds.bottom() > contentBounds_.y && bounds.y < contentBounds_.bottom()) {
            renderer.drawRoundedRect(bounds, theme.buttonBackground, theme.cornerRadius());
            renderer.drawRectOutline(bounds, theme.border);

            std::string text = (selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size()))
                ? items[selectedIndex] : "";
            renderer.drawText(text, bounds.x + theme.padding(),
                bounds.y + (bounds.h - theme.fontSize()) / 2,
                theme.text, theme.fontSize());

            renderer.drawText("v", bounds.right() - 16,
                bounds.y + (bounds.h - theme.fontSize()) / 2,
                theme.textDim, theme.fontSize());
        }

        return bounds;
    }

    // ============================================================================
    // TAB: GENERAL
    // ============================================================================

    void PreferencesWindow::drawGeneralTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float y = contentBounds_.y + theme.padding();
        int widgetId = 0;

        drawSectionHeader(renderer, y, "Project Settings");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Auto Save");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.autoSave,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Save Interval (min)");
            Rect fieldBounds = { controlBounds.x, controlBounds.y, 80, controlBounds.h };
            drawNumericField(renderer, fieldBounds, static_cast<float>(settings_.autoSaveInterval), "%.0f");
            hitAreas_.push_back({ fieldBounds, widgetId++, WidgetHitArea::Type::TextField });
        }

        drawSeparator(renderer, y);
        drawSectionHeader(renderer, y, "File Paths");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Default Path");
            drawTextField(renderer, controlBounds, settings_.projectPath, "(Not set)", false);
            hitAreas_.push_back({ controlBounds, widgetId++, WidgetHitArea::Type::TextField });
        }

        contentHeight_ = y - contentBounds_.y + theme.padding();
    }

    // ============================================================================
    // TAB: INTERFACE
    // ============================================================================

    void PreferencesWindow::drawInterfaceTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float y = contentBounds_.y + theme.padding();
        int widgetId = 100;

        drawSectionHeader(renderer, y, "Appearance");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "UI Scale");
            Rect sliderBounds = drawSlider(renderer, controlBounds, settings_.uiScale * 100,
                50.0f, 200.0f, "%.0f%%", hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ sliderBounds, widgetId++, WidgetHitArea::Type::Slider });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Theme");
            std::vector<std::string> themes = { "Dark", "Light", "Custom" };
            Rect dropBounds = { controlBounds.x, controlBounds.y, 120, controlBounds.h };
            drawDropdown(renderer, dropBounds, themes, settings_.themeIndex, false);
            hitAreas_.push_back({ dropBounds, widgetId++, WidgetHitArea::Type::Dropdown });
        }

        drawSeparator(renderer, y);
        drawSectionHeader(renderer, y, "Behavior");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Show Tooltips");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.showTooltips,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Animate UI");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.animateUI,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        contentHeight_ = y - contentBounds_.y + theme.padding();
    }

    // ============================================================================
    // TAB: VIEWPORT
    // ============================================================================

    void PreferencesWindow::drawViewportTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float y = contentBounds_.y + theme.padding();
        int widgetId = 200;

        drawSectionHeader(renderer, y, "Camera");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Field of View");
            drawSlider(renderer, controlBounds, settings_.fov, 30.0f, 120.0f, "%.0f",
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ controlBounds, widgetId++, WidgetHitArea::Type::Slider });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Near Clip");
            Rect fieldBounds = { controlBounds.x, controlBounds.y, 80, controlBounds.h };
            drawNumericField(renderer, fieldBounds, settings_.nearClip, "%.2f");
            hitAreas_.push_back({ fieldBounds, widgetId++, WidgetHitArea::Type::TextField });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Far Clip");
            Rect fieldBounds = { controlBounds.x, controlBounds.y, 80, controlBounds.h };
            drawNumericField(renderer, fieldBounds, settings_.farClip, "%.0f");
            hitAreas_.push_back({ fieldBounds, widgetId++, WidgetHitArea::Type::TextField });
        }

        drawSeparator(renderer, y);
        drawSectionHeader(renderer, y, "Display");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "VSync");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.vsync,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Show Grid");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.showGrid,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Show Axes");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.showAxes,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        contentHeight_ = y - contentBounds_.y + theme.padding();
    }

    // ============================================================================
    // TAB: INPUT
    // ============================================================================

    void PreferencesWindow::drawInputTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float y = contentBounds_.y + theme.padding();
        int widgetId = 300;

        drawSectionHeader(renderer, y, "Mouse");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Sensitivity");
            drawSlider(renderer, controlBounds, settings_.mouseSensitivity, 0.1f, 3.0f, "%.1f",
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ controlBounds, widgetId++, WidgetHitArea::Type::Slider });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Zoom Speed");
            drawSlider(renderer, controlBounds, settings_.zoomSpeed, 0.1f, 3.0f, "%.1f",
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ controlBounds, widgetId++, WidgetHitArea::Type::Slider });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Pan Speed");
            drawSlider(renderer, controlBounds, settings_.panSpeed, 0.1f, 3.0f, "%.1f",
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ controlBounds, widgetId++, WidgetHitArea::Type::Slider });
        }

        drawSeparator(renderer, y);
        drawSectionHeader(renderer, y, "Inversion");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Invert Y Axis");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.invertY,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Invert Zoom");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.invertZoom,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        contentHeight_ = y - contentBounds_.y + theme.padding();
    }

    // ============================================================================
    // TAB: PERFORMANCE
    // ============================================================================

    void PreferencesWindow::drawPerformanceTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float y = contentBounds_.y + theme.padding();
        int widgetId = 400;

        drawSectionHeader(renderer, y, "Memory");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Max Undo Steps");
            Rect fieldBounds = { controlBounds.x, controlBounds.y, 80, controlBounds.h };
            drawNumericField(renderer, fieldBounds, static_cast<float>(settings_.maxUndoSteps), "%.0f");
            hitAreas_.push_back({ fieldBounds, widgetId++, WidgetHitArea::Type::TextField });
        }

        drawSeparator(renderer, y);
        drawSectionHeader(renderer, y, "Rendering");

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "GPU Compute");
            Rect checkboxBounds = drawCheckbox(renderer, controlBounds, settings_.useGPUCompute,
                hoveredWidgetId_ == widgetId);
            hitAreas_.push_back({ checkboxBounds, widgetId++, WidgetHitArea::Type::Checkbox });
        }

        {
            Rect controlBounds = drawPropertyRow(renderer, y, "Texture Quality");
            std::vector<std::string> qualities = { "Low", "Medium", "High" };
            Rect dropBounds = { controlBounds.x, controlBounds.y, 120, controlBounds.h };
            drawDropdown(renderer, dropBounds, qualities, settings_.textureQuality, false);
            hitAreas_.push_back({ dropBounds, widgetId++, WidgetHitArea::Type::Dropdown });
        }

        contentHeight_ = y - contentBounds_.y + theme.padding();
    }

    // ============================================================================
    // TAB: CREDITS
    // ============================================================================

    void PreferencesWindow::drawCreditsTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float y = contentBounds_.y + theme.padding();
        float x = contentBounds_.x + theme.padding();

        auto drawLine = [&](const std::string& text, const Color& color) {
            float scrollY = y - currentScrollOffset_;
            if (scrollY + 20 > contentBounds_.y && scrollY < contentBounds_.bottom()) {
                renderer.drawText(text, x, scrollY, color, theme.fontSize());
            }
            y += 20;
            };

        auto drawLineIndented = [&](const std::string& text, const Color& color) {
            float scrollY = y - currentScrollOffset_;
            if (scrollY + 20 > contentBounds_.y && scrollY < contentBounds_.bottom()) {
                renderer.drawText(text, x + 20, scrollY, color, theme.fontSize());
            }
            y += 20;
            };

        drawLine("LIBRE DCC TOOL", theme.accent);
        drawLine("Version 0.1.0 (Development)", theme.textDim);
        y += 10;

        drawLine("A free, open-source digital content creation suite.", theme.text);
        y += 15;

        float scrollY = y - currentScrollOffset_;
        if (scrollY > contentBounds_.y && scrollY < contentBounds_.bottom()) {
            renderer.drawRect({
                x,
                scrollY,
                contentBounds_.w - theme.padding() * 2 - SCROLLBAR_WIDTH,
                1
                }, theme.border);
        }
        y += 15;

        drawLine("Core Development", theme.accent);
        y += 5;
        drawLineIndented("Lead Developer - [Your Name]", theme.text);
        drawLineIndented("Architecture & Design - [Your Name]", theme.text);
        y += 10;

        drawLine("Built With", theme.accent);
        y += 5;
        drawLineIndented("C++17, Vulkan, GLFW, GLM", theme.text);
        y += 10;

        scrollY = y - currentScrollOffset_;
        if (scrollY > contentBounds_.y && scrollY < contentBounds_.bottom()) {
            renderer.drawRect({
                x,
                scrollY,
                contentBounds_.w - theme.padding() * 2 - SCROLLBAR_WIDTH,
                1
                }, theme.border);
        }
        y += 15;

        drawLine("License", theme.accent);
        y += 5;
        drawLineIndented("This software is open source.", theme.text);
        drawLineIndented("See LICENSE file for details.", theme.textDim);
        y += 15;

        drawLine("github.com/your-repo/libre-dcc", theme.textDim);

        contentHeight_ = y - contentBounds_.y + theme.padding();
    }

    // ============================================================================
    // INPUT HANDLING
    // ============================================================================

    bool PreferencesWindow::handleMouse(const MouseEvent& event) {
        if (!isOpen) return false;

        // Let base Window handle title bar, close button, dragging
        if (Window::handleMouse(event)) {
            return true;
        }

        // Handle tab buttons
        for (auto& btn : tabButtons_) {
            btn.hovered = btn.bounds.contains(event.x, event.y);

            if (btn.hovered && event.pressed && event.button == MouseButton::Left) {
                currentTab_ = btn.tab;
                return true;
            }
        }

        // Handle scrollbar dragging
        if (scrollbarDragging_) {
            if (event.released) {
                scrollbarDragging_ = false;
            }
            else {
                float maxScroll = contentHeight_ - contentBounds_.h;
                if (maxScroll < 0) maxScroll = 0;
                if (maxScroll > 0) {
                    float scrollRange = contentBounds_.h - scrollbarThumbBounds_.h;
                    float delta = event.y - scrollDragStartY_;
                    scrollOffsets_[currentTab_] = scrollDragStartOffset_ + (delta / scrollRange) * maxScroll;
                    scrollOffsets_[currentTab_] = clampVal(scrollOffsets_[currentTab_], 0.0f, maxScroll);
                }
            }
            return true;
        }

        // Start scrollbar drag
        if (scrollbarThumbBounds_.contains(event.x, event.y) && event.pressed && event.button == MouseButton::Left) {
            scrollbarDragging_ = true;
            scrollDragStartY_ = event.y;
            scrollDragStartOffset_ = scrollOffsets_[currentTab_];
            return true;
        }

        // Handle scroll wheel
        if (contentBounds_.contains(event.x, event.y) && event.scroll != 0) {
            float maxScroll = contentHeight_ - contentBounds_.h;
            if (maxScroll > 0) {
                scrollOffsets_[currentTab_] -= event.scroll * 30.0f;
                scrollOffsets_[currentTab_] = clampVal(scrollOffsets_[currentTab_], 0.0f, maxScroll);
            }
            return true;
        }

        // Update hovered widget
        hoveredWidgetId_ = -1;
        for (auto& area : hitAreas_) {
            if (area.bounds.contains(event.x, event.y)) {
                hoveredWidgetId_ = area.id;

                // Handle clicks on widgets
                if (event.pressed && event.button == MouseButton::Left) {
                    if (area.type == WidgetHitArea::Type::Checkbox) {
                        // Toggle the appropriate checkbox based on widget ID
                        handleCheckboxClick(area.id);
                        return true;
                    }
                }
                break;
            }
        }

        return bounds.contains(event.x, event.y);
    }

    void PreferencesWindow::handleCheckboxClick(int widgetId) {
        // General tab (0-99)
        if (widgetId == 0) settings_.autoSave = !settings_.autoSave;

        // Interface tab (100-199)
        else if (widgetId == 102) settings_.showTooltips = !settings_.showTooltips;
        else if (widgetId == 103) settings_.animateUI = !settings_.animateUI;

        // Viewport tab (200-299)
        else if (widgetId == 203) settings_.vsync = !settings_.vsync;
        else if (widgetId == 204) settings_.showGrid = !settings_.showGrid;
        else if (widgetId == 205) settings_.showAxes = !settings_.showAxes;

        // Input tab (300-399)
        else if (widgetId == 303) settings_.invertY = !settings_.invertY;
        else if (widgetId == 304) settings_.invertZoom = !settings_.invertZoom;

        // Performance tab (400-499)
        else if (widgetId == 401) settings_.useGPUCompute = !settings_.useGPUCompute;
    }

    bool PreferencesWindow::handleKey(const KeyEvent& event) {
        if (!isOpen) return false;

        // Escape closes window
        if (event.pressed && event.key == 256) {  // GLFW_KEY_ESCAPE
            isOpen = false;
            return true;
        }

        return false;
    }

} // namespace libre::ui