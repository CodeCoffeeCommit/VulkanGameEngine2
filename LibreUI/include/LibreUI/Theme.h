// LibreUI/include/LibreUI/Theme.h
// UI theming with DPI-scaled values

#pragma once

#include <LibreUI/Types.h>

#include <LibreUI/Export.h>


namespace LibreUI {

    struct LIBREUI_API Theme {
        // ====================================================================
        // COLORS (not scaled)
        // ====================================================================

        // Backgrounds
        Color background{ 0.22f, 0.22f, 0.22f, 1.0f };
        Color backgroundDark{ 0.18f, 0.18f, 0.18f, 1.0f };
        Color backgroundLight{ 0.26f, 0.26f, 0.26f, 1.0f };

        // Panels
        Color panelHeader{ 0.25f, 0.25f, 0.25f, 1.0f };
        Color panelHeaderHover{ 0.30f, 0.30f, 0.30f, 1.0f };

        // Buttons
        Color buttonBackground{ 0.30f, 0.30f, 0.30f, 1.0f };
        Color buttonHover{ 0.35f, 0.35f, 0.35f, 1.0f };
        Color buttonPressed{ 0.25f, 0.25f, 0.25f, 1.0f };

        // Text
        Color text{ 0.90f, 0.90f, 0.90f, 1.0f };
        Color textDim{ 0.60f, 0.60f, 0.60f, 1.0f };

        // Accents
        Color accent{ 0.26f, 0.52f, 0.96f, 1.0f };
        Color accentHover{ 0.36f, 0.62f, 1.00f, 1.0f };

        // Borders
        Color border{ 0.15f, 0.15f, 0.15f, 1.0f };

        // Scrollbar
        Color scrollbarTrack{ 0.20f, 0.20f, 0.20f, 1.0f };
        Color scrollbarThumb{ 0.40f, 0.40f, 0.40f, 1.0f };

        // ====================================================================
        // BASE SIZES (in abstract units, will be scaled)
        // ====================================================================

        float baseFontSize = 13.0f;
        float baseFontSizeSmall = 11.0f;
        float baseFontSizeLarge = 16.0f;

        float baseCornerRadius = 4.0f;
        float basePadding = 8.0f;
        float baseSpacing = 4.0f;

        float basePanelHeaderHeight = 26.0f;
        float baseButtonHeight = 24.0f;
        float baseDropdownItemHeight = 24.0f;

        float baseSliderHeight = 20.0f;
        float baseSliderTrackHeight = 4.0f;
        float baseSliderHandleWidth = 12.0f;

        float baseCheckboxSize = 16.0f;
        float baseTextFieldHeight = 24.0f;

        float baseScrollbarWidth = 12.0f;
        float baseScrollbarMinThumbHeight = 20.0f;

        float basePropertyLabelWidth = 120.0f;
        float basePropertyRowHeight = 24.0f;

        // ====================================================================
        // SCALED GETTERS (implemented in Theme.cpp)
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
    };

    // Global theme accessor
    LIBREUI_API Theme& GetTheme();

} // namespace LibreUI