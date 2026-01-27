// LibreUI/src/Core/Theme.cpp
// Implementation of scaled theme getters

#include <LibreUI/Theme.h>
#include <LibreUI/Scale.h>

namespace LibreUI {

    // ========================================================================
    // SCALED GETTERS
    // Each getter multiplies the base value by the current scale factor
    // ========================================================================

    float Theme::fontSize() const {
        return Scale::instance().toPixels(baseFontSize);
    }

    float Theme::fontSizeSmall() const {
        return Scale::instance().toPixels(baseFontSizeSmall);
    }

    float Theme::fontSizeLarge() const {
        return Scale::instance().toPixels(baseFontSizeLarge);
    }

    float Theme::cornerRadius() const {
        return Scale::instance().toPixels(baseCornerRadius);
    }

    float Theme::padding() const {
        return Scale::instance().toPixels(basePadding);
    }

    float Theme::spacing() const {
        return Scale::instance().toPixels(baseSpacing);
    }

    float Theme::panelHeaderHeight() const {
        return Scale::instance().toPixels(basePanelHeaderHeight);
    }

    float Theme::buttonHeight() const {
        return Scale::instance().toPixels(baseButtonHeight);
    }

    float Theme::dropdownItemHeight() const {
        return Scale::instance().toPixels(baseDropdownItemHeight);
    }

    float Theme::sliderHeight() const {
        return Scale::instance().toPixels(baseSliderHeight);
    }

    float Theme::sliderTrackHeight() const {
        return Scale::instance().toPixels(baseSliderTrackHeight);
    }

    float Theme::sliderHandleWidth() const {
        return Scale::instance().toPixels(baseSliderHandleWidth);
    }

    float Theme::checkboxSize() const {
        return Scale::instance().toPixels(baseCheckboxSize);
    }

    float Theme::textFieldHeight() const {
        return Scale::instance().toPixels(baseTextFieldHeight);
    }

    float Theme::scrollbarWidth() const {
        return Scale::instance().toPixels(baseScrollbarWidth);
    }

    float Theme::scrollbarMinThumbHeight() const {
        return Scale::instance().toPixels(baseScrollbarMinThumbHeight);
    }

    float Theme::propertyLabelWidth() const {
        return Scale::instance().toPixels(basePropertyLabelWidth);
    }

    float Theme::propertyRowHeight() const {
        return Scale::instance().toPixels(basePropertyRowHeight);
    }

    // ========================================================================
    // GLOBAL THEME INSTANCE
    // ========================================================================

    static Theme globalTheme;

    Theme& GetTheme() {
        return globalTheme;
    }

} // namespace LibreUI