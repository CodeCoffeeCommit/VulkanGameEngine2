// src/ui/Theme.h
// UI Theme with Blender-style DPI scaling
// Colors matched to Blender 4.x/5.x default dark theme

#pragma once

#include "Core.h"

namespace libre::ui {

    struct Theme {
        // ====================================================================
        // BASE VALUES (Abstract units at scale factor 1.0)
        // Matched to Blender's default sizes
        // ====================================================================

        // Font sizes (Blender uses 11-12pt for UI)
        float baseFontSize = 12.0f;
        float baseFontSizeSmall = 10.0f;
        float baseFontSizeLarge = 14.0f;

        // Widget dimensions (Blender style)
        float baseCornerRadius = 4.0f;
        float basePadding = 6.0f;
        float baseSpacing = 2.0f;

        // Panel
        float basePanelHeaderHeight = 24.0f;

        // Button
        float baseButtonHeight = 20.0f;

        // Dropdown
        float baseDropdownItemHeight = 20.0f;

        // Slider
        float baseSliderHeight = 20.0f;
        float baseSliderTrackHeight = 3.0f;
        float baseSliderHandleWidth = 10.0f;

        // Checkbox
        float baseCheckboxSize = 14.0f;

        // TextField
        float baseTextFieldHeight = 20.0f;

        // Scrollbar
        float baseScrollbarWidth = 6.0f;
        float baseScrollbarMinThumbHeight = 20.0f;

        // Property row
        float basePropertyLabelWidth = 100.0f;
        float basePropertyRowHeight = 22.0f;

        // ====================================================================
        // SCALED GETTERS (Call these when drawing/laying out)
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
        // COLORS - Blender 4.x/5.x Default Dark Theme
        // ====================================================================

        // Main backgrounds (Blender: #282828, #232323, #303030)
        Color background{ 0.157f, 0.157f, 0.157f, 1.0f };        // #282828
        Color backgroundDark{ 0.137f, 0.137f, 0.137f, 1.0f };    // #232323
        Color backgroundLight{ 0.188f, 0.188f, 0.188f, 1.0f };   // #303030

        // Text colors (Blender: #E5E5E5, #9A9A9A)
        Color text{ 0.898f, 0.898f, 0.898f, 1.0f };              // #E5E5E5
        Color textDim{ 0.604f, 0.604f, 0.604f, 1.0f };           // #9A9A9A

        // Accent colors - Blender Blue (#4772B3, #5680C2)
        Color accent{ 0.278f, 0.447f, 0.702f, 1.0f };            // #4772B3
        Color accentHover{ 0.337f, 0.502f, 0.761f, 1.0f };       // #5680C2

        // Border (Blender: #1A1A1A)
        Color border{ 0.102f, 0.102f, 0.102f, 1.0f };            // #1A1A1A

        // Panel header (Blender: #2D2D2D, #353535)
        Color panelHeader{ 0.176f, 0.176f, 0.176f, 1.0f };       // #2D2D2D
        Color panelHeaderHover{ 0.208f, 0.208f, 0.208f, 1.0f };  // #353535

        // Button (Blender: #424242, #4A4A4A, #3A3A3A)
        Color buttonBackground{ 0.259f, 0.259f, 0.259f, 1.0f };  // #424242
        Color buttonHover{ 0.290f, 0.290f, 0.290f, 1.0f };       // #4A4A4A
        Color buttonPressed{ 0.227f, 0.227f, 0.227f, 1.0f };     // #3A3A3A

        // Dropdown (Blender: #1F1F1F, #333333)
        Color dropdownBackground{ 0.122f, 0.122f, 0.122f, 1.0f }; // #1F1F1F
        Color dropdownItemHover{ 0.2f, 0.2f, 0.2f, 1.0f };        // #333333

        // Slider (Blender uses accent color for fill)
        Color sliderTrack{ 0.122f, 0.122f, 0.122f, 1.0f };       // #1F1F1F
        Color sliderFill{ 0.278f, 0.447f, 0.702f, 1.0f };        // #4772B3 (accent)

        // Checkbox
        Color checkboxBackground{ 0.176f, 0.176f, 0.176f, 1.0f }; // #2D2D2D
        Color checkboxCheck{ 0.278f, 0.447f, 0.702f, 1.0f };      // #4772B3 (accent)

        // TextField (Blender: #1D1D1D, #454545)
        Color textFieldBackground{ 0.114f, 0.114f, 0.114f, 1.0f }; // #1D1D1D
        Color textFieldBorder{ 0.271f, 0.271f, 0.271f, 1.0f };     // #454545
        Color textFieldFocusBorder{ 0.278f, 0.447f, 0.702f, 1.0f }; // #4772B3 (accent)
        Color textFieldPlaceholder{ 0.4f, 0.4f, 0.4f, 1.0f };      // #666666
        Color textFieldCursor{ 0.898f, 0.898f, 0.898f, 1.0f };     // #E5E5E5

        // Scrollbar (Blender: #1A1A1A, #5A5A5A, #707070)
        Color scrollbarTrack{ 0.102f, 0.102f, 0.102f, 1.0f };    // #1A1A1A
        Color scrollbarThumb{ 0.353f, 0.353f, 0.353f, 1.0f };    // #5A5A5A
        Color scrollbarThumbHover{ 0.439f, 0.439f, 0.439f, 1.0f }; // #707070

        // Separator (Blender: #1A1A1A)
        Color separatorColor{ 0.102f, 0.102f, 0.102f, 1.0f };    // #1A1A1A

        // Section header
        Color sectionHeaderBackground{ 0.176f, 0.176f, 0.176f, 1.0f }; // #2D2D2D
        Color sectionHeaderHover{ 0.208f, 0.208f, 0.208f, 1.0f };      // #353535

        // ====================================================================
        // ADDITIONAL BLENDER COLORS (for future use)
        // ====================================================================

        // Selection/Active state
        Color selection{ 0.278f, 0.447f, 0.702f, 0.5f };         // #4772B3 at 50%

        // Error/Warning/Success
        Color error{ 0.8f, 0.2f, 0.2f, 1.0f };                   // Red
        Color warning{ 0.9f, 0.7f, 0.2f, 1.0f };                 // Yellow/Orange
        Color success{ 0.2f, 0.7f, 0.3f, 1.0f };                 // Green

        // Viewport background (Blender 3D view gradient)
        Color viewportTop{ 0.225f, 0.225f, 0.225f, 1.0f };       // #393939
        Color viewportBottom{ 0.157f, 0.157f, 0.157f, 1.0f };    // #282828

        // Wire/Grid colors
        Color gridColor{ 0.282f, 0.282f, 0.282f, 1.0f };         // #484848
        Color wireColor{ 0.0f, 0.0f, 0.0f, 1.0f };               // Black
    };

    // Global theme access
    Theme& GetTheme();

} // namespace libre::ui