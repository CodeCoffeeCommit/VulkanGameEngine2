#include "PreferencesWindow.h"
#include "Theme.h"

namespace libre::ui {

    PreferencesWindow::PreferencesWindow() : Window("Preferences") {
        // Initialize tab buttons
        tabButtons_ = {
            { "General",   PreferencesTab::General,   {}, false },
            { "Interface", PreferencesTab::Interface, {}, false },
            { "Viewport",  PreferencesTab::Viewport,  {}, false },
            { "Credits",   PreferencesTab::Credits,   {}, false }
        };
        
        // Default window size
        bounds = { 0, 0, 500, 400 };
        closable = true;
        draggable = true;
        isOpen = false;
    }

    void PreferencesWindow::layout(const Rect& available) {
        // First, call parent layout for title bar, close button, etc.
        Window::layout(available);

        auto& theme = GetTheme();
        
        // Calculate internal regions (below title bar)
        float contentTop = bounds.y + theme.panelHeaderHeight;
        float contentHeight = bounds.h - theme.panelHeaderHeight;
        
        // Sidebar on the left
        sidebarBounds_ = {
            bounds.x,
            contentTop,
            SIDEBAR_WIDTH,
            contentHeight
        };
        
        // Main content area on the right
        contentBounds_ = {
            bounds.x + SIDEBAR_WIDTH,
            contentTop,
            bounds.w - SIDEBAR_WIDTH,
            contentHeight
        };
        
        // Layout tab buttons in sidebar
        float y = sidebarBounds_.y + theme.padding;
        for (auto& btn : tabButtons_) {
            btn.bounds = {
                sidebarBounds_.x + theme.padding,
                y,
                SIDEBAR_WIDTH - theme.padding * 2,
                theme.buttonHeight
            };
            y += theme.buttonHeight + theme.spacing;
        }
    }

    void PreferencesWindow::draw(UIRenderer& renderer) {
        if (!isOpen) return;

        // Draw the base window (shadow, background, title bar, close button)
        Window::draw(renderer);
        
        // Draw our custom content
        drawSidebar(renderer);
        drawContent(renderer);
    }

    void PreferencesWindow::drawSidebar(UIRenderer& renderer) {
        auto& theme = GetTheme();
        
        // Sidebar background (slightly darker)
        renderer.drawRect(sidebarBounds_, theme.backgroundDark);
        
        // Separator line
        renderer.drawRect({
            sidebarBounds_.right() - 1,
            sidebarBounds_.y,
            1,
            sidebarBounds_.h
        }, theme.border);
        
        // Tab buttons
        for (const auto& btn : tabButtons_) {
            bool isActive = (btn.tab == currentTab_);
            
            Color bgColor;
            if (isActive) {
                bgColor = theme.accent;
            } else if (btn.hovered) {
                bgColor = theme.buttonHover;
            } else {
                bgColor = theme.buttonBackground;
            }
            
            renderer.drawRoundedRect(btn.bounds, bgColor, theme.cornerRadius);
            
            // Center text in button
            Vec2 textSize = renderer.measureText(btn.label, theme.fontSize);
            float textX = btn.bounds.x + (btn.bounds.w - textSize.x) / 2;
            float textY = btn.bounds.y + (btn.bounds.h - textSize.y) / 2;
            renderer.drawText(btn.label, textX, textY, theme.text, theme.fontSize);
        }
    }

    void PreferencesWindow::drawContent(UIRenderer& renderer) {
        auto& theme = GetTheme();
        
        // Content background
        renderer.drawRect(contentBounds_, theme.background);
        
        // Push clip to content area
        renderer.pushClip(contentBounds_);
        
        // Draw content based on current tab
        switch (currentTab_) {
            case PreferencesTab::General:
                drawGeneralTab(renderer);
                break;
            case PreferencesTab::Interface:
                drawInterfaceTab(renderer);
                break;
            case PreferencesTab::Viewport:
                drawViewportTab(renderer);
                break;
            case PreferencesTab::Credits:
                drawCreditsTab(renderer);
                break;
        }
        
        renderer.popClip();
    }

    void PreferencesWindow::drawGeneralTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float x = contentBounds_.x + theme.padding;
        float y = contentBounds_.y + theme.padding;
        
        renderer.drawText("General Settings", x, y, theme.text, theme.fontSize + 2);
        y += 30;
        
        renderer.drawText("Project defaults and general behavior.", x, y, theme.textDim, theme.fontSize);
        y += 25;
        
        renderer.drawText("(Settings coming soon)", x, y, theme.textDim, theme.fontSize);
    }

    void PreferencesWindow::drawInterfaceTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float x = contentBounds_.x + theme.padding;
        float y = contentBounds_.y + theme.padding;
        
        renderer.drawText("Interface Settings", x, y, theme.text, theme.fontSize + 2);
        y += 30;
        
        renderer.drawText("Theme, colors, and UI customization.", x, y, theme.textDim, theme.fontSize);
        y += 25;
        
        renderer.drawText("(Settings coming soon)", x, y, theme.textDim, theme.fontSize);
    }

    void PreferencesWindow::drawViewportTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float x = contentBounds_.x + theme.padding;
        float y = contentBounds_.y + theme.padding;
        
        renderer.drawText("Viewport Settings", x, y, theme.text, theme.fontSize + 2);
        y += 30;
        
        renderer.drawText("3D viewport behavior and display options.", x, y, theme.textDim, theme.fontSize);
        y += 25;
        
        renderer.drawText("(Settings coming soon)", x, y, theme.textDim, theme.fontSize);
    }

    void PreferencesWindow::drawCreditsTab(UIRenderer& renderer) {
        auto& theme = GetTheme();
        float x = contentBounds_.x + theme.padding;
        float y = contentBounds_.y + theme.padding;
        float lineHeight = theme.fontSize + 6;
        
        // Title
        renderer.drawText("Libre DCC Tool", x, y, theme.accent, theme.fontSize + 4);
        y += lineHeight + 10;
        
        // Version
        renderer.drawText("Version 0.1.0 (Development Build)", x, y, theme.textDim, theme.fontSize);
        y += lineHeight + 15;
        
        // Description
        renderer.drawText("A free and open-source digital content creation suite.", x, y, theme.text, theme.fontSize);
        y += lineHeight + 20;
        
        // Separator
        renderer.drawRect({ x, y, contentBounds_.w - theme.padding * 2, 1 }, theme.border);
        y += 15;
        
        // Core Team
        renderer.drawText("Core Development", x, y, theme.accent, theme.fontSize);
        y += lineHeight + 5;
        
        // Add your credits here - these are placeholders
        renderer.drawText("Lead Developer - [Your Name]", x + 10, y, theme.text, theme.fontSize);
        y += lineHeight;
        
        renderer.drawText("Architecture & Design - [Your Name]", x + 10, y, theme.text, theme.fontSize);
        y += lineHeight + 15;
        
        // Technologies
        renderer.drawText("Built With", x, y, theme.accent, theme.fontSize);
        y += lineHeight + 5;
        
        renderer.drawText("C++17, Vulkan, GLFW, GLM", x + 10, y, theme.text, theme.fontSize);
        y += lineHeight + 15;
        
        // Separator
        renderer.drawRect({ x, y, contentBounds_.w - theme.padding * 2, 1 }, theme.border);
        y += 15;
        
        // License
        renderer.drawText("License", x, y, theme.accent, theme.fontSize);
        y += lineHeight + 5;
        
        renderer.drawText("This software is open source.", x + 10, y, theme.text, theme.fontSize);
        y += lineHeight;
        
        renderer.drawText("See LICENSE file for details.", x + 10, y, theme.textDim, theme.fontSize);
        y += lineHeight + 20;
        
        // Website/Contact (placeholder)
        renderer.drawText("github.com/your-repo/libre-dcc", x, y, theme.textDim, theme.fontSize);
    }

    bool PreferencesWindow::handleMouse(const MouseEvent& event) {
        if (!isOpen) return false;
        
        // First, let parent handle title bar dragging and close button
        if (Window::handleMouse(event)) {
            return true;
        }
        
        // Handle tab button clicks
        for (auto& btn : tabButtons_) {
            btn.hovered = btn.bounds.contains(event.x, event.y);
            
            if (btn.hovered && event.pressed && event.button == MouseButton::Left) {
                currentTab_ = btn.tab;
                return true;
            }
        }
        
        // Consume events within our bounds
        return bounds.contains(event.x, event.y);
    }

} // namespace libre::ui