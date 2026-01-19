// src/ui/Theme.cpp
// Implementation of scaled theme getters

#include "Theme.h"
#include "UIScale.h"

namespace libre::ui {

    // ========================================================================
    // SCALED GETTERS
    // Each getter multiplies the base value by the current scale factor
    // ========================================================================

    float Theme::fontSize() const {
        return UIScale::instance().toPixels(baseFontSize);
    }

    float Theme::fontSizeSmall() const {
        return UIScale::instance().toPixels(baseFontSizeSmall);
    }

    float Theme::fontSizeLarge() const {
        return UIScale::instance().toPixels(baseFontSizeLarge);
    }

    float Theme::cornerRadius() const {
        return UIScale::instance().toPixels(baseCornerRadius);
    }

    float Theme::padding() const {
        return UIScale::instance().toPixels(basePadding);
    }

    float Theme::spacing() const {
        return UIScale::instance().toPixels(baseSpacing);
    }

    float Theme::panelHeaderHeight() const {
        return UIScale::instance().toPixels(basePanelHeaderHeight);
    }

    float Theme::buttonHeight() const {
        return UIScale::instance().toPixels(baseButtonHeight);
    }

    float Theme::dropdownItemHeight() const {
        return UIScale::instance().toPixels(baseDropdownItemHeight);
    }

    float Theme::sliderHeight() const {
        return UIScale::instance().toPixels(baseSliderHeight);
    }

    float Theme::sliderTrackHeight() const {
        return UIScale::instance().toPixels(baseSliderTrackHeight);
    }

    float Theme::sliderHandleWidth() const {
        return UIScale::instance().toPixels(baseSliderHandleWidth);
    }

    float Theme::checkboxSize() const {
        return UIScale::instance().toPixels(baseCheckboxSize);
    }

    float Theme::textFieldHeight() const {
        return UIScale::instance().toPixels(baseTextFieldHeight);
    }

    float Theme::scrollbarWidth() const {
        return UIScale::instance().toPixels(baseScrollbarWidth);
    }

    float Theme::scrollbarMinThumbHeight() const {
        return UIScale::instance().toPixels(baseScrollbarMinThumbHeight);
    }

    float Theme::propertyLabelWidth() const {
        return UIScale::instance().toPixels(basePropertyLabelWidth);
    }

    float Theme::propertyRowHeight() const {
        return UIScale::instance().toPixels(basePropertyRowHeight);
    }

    // ========================================================================
    // GLOBAL THEME INSTANCE
    // ========================================================================

    static Theme globalTheme;

    Theme& GetTheme() {
        return globalTheme;
    }

} // namespace libre::ui