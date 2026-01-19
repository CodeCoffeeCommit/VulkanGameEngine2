// src/ui/Theme.h
// UI Theme with Blender-style DPI scaling
//
// Base values are stored in abstract units (design values at scale 1.0).
// Scaled getters return actual pixel values by multiplying with the scale factor.
//
// Usage:
//   float height = GetTheme().buttonHeight();  // Returns scaled pixels
//   float baseH = GetTheme().baseButtonHeight; // Returns abstract units

#pragma once

#include "Core.h"

namespace libre::ui {

    struct Theme {
        // ====================================================================
        // BASE VALUES (Abstract units at scale factor 1.0)
        // These define the "design" dimensions before DPI scaling
        // ====================================================================

        // Font sizes (in points)
        float baseFontSize = 13.0f;
        float baseFontSizeSmall = 11.0f;
        float baseFontSizeLarge = 16.0f;

        // Widget dimensions
        float baseCornerRadius = 4.0f;
        float basePadding = 8.0f;
        float baseSpacing = 4.0f;

        // Panel
        float basePanelHeaderHeight = 26.0f;

        // Button
        float baseButtonHeight = 24.0f;

        // Dropdown
        float baseDropdownItemHeight = 24.0f;

        // Slider
        float baseSliderHeight = 24.0f;
        float baseSliderTrackHeight = 4.0f;
        float baseSliderHandleWidth = 12.0f;

        // Checkbox
        float baseCheckboxSize = 16.0f;

        // TextField
        float baseTextFieldHeight = 28.0f;

        // Scrollbar
        float baseScrollbarWidth = 8.0f;
        float baseScrollbarMinThumbHeight = 20.0f;

        // Property row
        float basePropertyLabelWidth = 120.0f;
        float basePropertyRowHeight = 26.0f;

        // ====================================================================
        // SCALED GETTERS (Call these when drawing/laying out)
        // These return actual pixel values after DPI scaling
        // ====================================================================

        float fontSize() const;
        float fontSizeSmall() const;
        float fontSizeLarge() const;
        float cornerRadius() const;
        float padding() const;
        float spacing() const;
        float panelHeaderHeight() const;
        float buttonHeight() const;
        float dropdownItemHeight() const;
        float sliderHeight() const;
        float sliderTrackHeight() const;
        float sliderHandleWidth() const;
        float checkboxSize() const;
        float textFieldHeight() const;
        float scrollbarWidth() const;
        float scrollbarMinThumbHeight() const;
        float propertyLabelWidth() const;
        float propertyRowHeight() const;

        // ====================================================================
        // COLORS (Not scaled - colors don't change with DPI)
        // ====================================================================

        // Background colors
        Color background{ 0.22f, 0.22f, 0.22f, 1.0f };
        Color backgroundDark{ 0.18f, 0.18f, 0.18f, 1.0f };
        Color backgroundLight{ 0.28f, 0.28f, 0.28f, 1.0f };

        // Text colors
        Color text{ 0.9f, 0.9f, 0.9f, 1.0f };
        Color textDim{ 0.6f, 0.6f, 0.6f, 1.0f };

        // Accent colors
        Color accent{ 0.3f, 0.5f, 0.8f, 1.0f };
        Color accentHover{ 0.4f, 0.6f, 0.9f, 1.0f };

        // Border
        Color border{ 0.1f, 0.1f, 0.1f, 1.0f };

        // Panel
        Color panelHeader{ 0.25f, 0.25f, 0.25f, 1.0f };
        Color panelHeaderHover{ 0.3f, 0.3f, 0.3f, 1.0f };

        // Button
        Color buttonBackground{ 0.3f, 0.3f, 0.3f, 1.0f };
        Color buttonHover{ 0.35f, 0.35f, 0.35f, 1.0f };
        Color buttonPressed{ 0.25f, 0.25f, 0.25f, 1.0f };

        // Dropdown
        Color dropdownBackground{ 0.2f, 0.2f, 0.2f, 1.0f };
        Color dropdownItemHover{ 0.3f, 0.3f, 0.3f, 1.0f };

        // Slider
        Color sliderTrack{ 0.15f, 0.15f, 0.15f, 1.0f };
        Color sliderFill{ 0.3f, 0.5f, 0.8f, 1.0f };

        // Checkbox
        Color checkboxBackground{ 0.2f, 0.2f, 0.2f, 1.0f };
        Color checkboxCheck{ 0.3f, 0.5f, 0.8f, 1.0f };

        // TextField
        Color textFieldBackground{ 0.18f, 0.18f, 0.18f, 1.0f };
        Color textFieldBorder{ 0.3f, 0.3f, 0.3f, 1.0f };
        Color textFieldFocusBorder{ 0.3f, 0.5f, 0.8f, 1.0f };
        Color textFieldPlaceholder{ 0.5f, 0.5f, 0.5f, 1.0f };
        Color textFieldCursor{ 0.9f, 0.9f, 0.9f, 1.0f };

        // Scrollbar
        Color scrollbarTrack{ 0.15f, 0.15f, 0.15f, 1.0f };
        Color scrollbarThumb{ 0.35f, 0.35f, 0.35f, 1.0f };
        Color scrollbarThumbHover{ 0.45f, 0.45f, 0.45f, 1.0f };

        // Separator
        Color separatorColor{ 0.25f, 0.25f, 0.25f, 1.0f };

        // Section header
        Color sectionHeaderBackground{ 0.2f, 0.2f, 0.2f, 1.0f };
        Color sectionHeaderHover{ 0.25f, 0.25f, 0.25f, 1.0f };
    };

    // Global theme access
    Theme& GetTheme();

} // namespace libre::ui