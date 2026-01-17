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
    };

    // Global theme access
    inline Theme& GetTheme() {
        static Theme theme;
        return theme;
    }

} // namespace libre::ui