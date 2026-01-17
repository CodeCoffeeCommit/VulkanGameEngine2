#pragma once

#include "Core.h"

namespace libre::ui {

    struct Theme {
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

        // Widget settings
        float cornerRadius = 4.0f;
        float padding = 8.0f;
        float spacing = 4.0f;

        // Panel
        float panelHeaderHeight = 26.0f;
        Color panelHeader{ 0.25f, 0.25f, 0.25f, 1.0f };
        Color panelHeaderHover{ 0.3f, 0.3f, 0.3f, 1.0f };

        // Button
        float buttonHeight = 24.0f;
        Color buttonBackground{ 0.3f, 0.3f, 0.3f, 1.0f };
        Color buttonHover{ 0.35f, 0.35f, 0.35f, 1.0f };
        Color buttonPressed{ 0.25f, 0.25f, 0.25f, 1.0f };

        // Dropdown
        float dropdownItemHeight = 24.0f;
        Color dropdownBackground{ 0.2f, 0.2f, 0.2f, 1.0f };
        Color dropdownItemHover{ 0.3f, 0.3f, 0.3f, 1.0f };

        // Font
        float fontSize = 13.0f;
               
        // Slider
        float sliderHeight = 24.0f;
        float sliderTrackHeight = 4.0f;
        float sliderHandleWidth = 12.0f;
        Color sliderTrack{ 0.15f, 0.15f, 0.15f, 1.0f };
        Color sliderFill{ 0.3f, 0.5f, 0.8f, 1.0f };  // Same as accent

        // Checkbox
        float checkboxSize = 16.0f;
        Color checkboxBackground{ 0.2f, 0.2f, 0.2f, 1.0f };
        Color checkboxCheck{ 0.3f, 0.5f, 0.8f, 1.0f };  // Same as accent

        // TextField
        float textFieldHeight = 28.0f;
        Color textFieldBackground{ 0.18f, 0.18f, 0.18f, 1.0f };
        Color textFieldBorder{ 0.3f, 0.3f, 0.3f, 1.0f };
        Color textFieldFocusBorder{ 0.3f, 0.5f, 0.8f, 1.0f };  // Same as accent
        Color textFieldPlaceholder{ 0.5f, 0.5f, 0.5f, 1.0f };
        Color textFieldCursor{ 0.9f, 0.9f, 0.9f, 1.0f };

        // Scrollbar
        float scrollbarWidth = 8.0f;
        float scrollbarMinThumbHeight = 20.0f;
        Color scrollbarTrack{ 0.15f, 0.15f, 0.15f, 1.0f };
        Color scrollbarThumb{ 0.35f, 0.35f, 0.35f, 1.0f };
        Color scrollbarThumbHover{ 0.45f, 0.45f, 0.45f, 1.0f };

        // Separator
        Color separatorColor{ 0.25f, 0.25f, 0.25f, 1.0f };

        // Property row
        float propertyLabelWidth = 120.0f;
        float propertyRowHeight = 26.0f;

        // Section header
        Color sectionHeaderBackground{ 0.2f, 0.2f, 0.2f, 1.0f };
        Color sectionHeaderHover{ 0.25f, 0.25f, 0.25f, 1.0f };
    };

    // Global theme access
    inline Theme& GetTheme() {
        static Theme theme;
        return theme;
    }

} // namespace libre::ui