// LibreUI/include/LibreUI/Scale.h
// DPI-aware scaling system - NO PLATFORM DEPENDENCIES
// App provides the scale factor, LibreUI just uses it

#pragma once

#include <algorithm>

#include <LibreUI/Export.h>

namespace LibreUI {

    class LIBREUI_API Scale {
    public:
        // Singleton access
        static Scale& instance() {
            static Scale inst;
            return inst;
        }

        // ====================================================================
        // INITIALIZATION - Called by app with platform-detected DPI scale
        // ====================================================================

        // App calls this once at startup with the DPI scale from the platform
        // Windows: 1.0 = 100%, 1.25 = 125%, 1.5 = 150%, 2.0 = 200%
        void initialize(float systemDpiScale) {
            systemScale_ = systemDpiScale;
            updateScaleFactor();
            initialized_ = true;
        }

        // Call if window moves to different monitor
        void setSystemScale(float scale) {
            if (scale != systemScale_) {
                systemScale_ = scale;
                updateScaleFactor();
            }
        }

        // ====================================================================
        // USER PREFERENCES
        // ====================================================================

        void setUserScale(float scale) {
            userScale_ = std::clamp(scale, 0.5f, 3.0f);
            updateScaleFactor();
        }

        float getUserScale() const { return userScale_; }

        // ====================================================================
        // CONVERSION
        // ====================================================================

        // Convert abstract units to pixels
        float toPixels(float abstractUnits) const {
            return abstractUnits * scaleFactor_;
        }

        // Convert pixels to abstract units
        float toAbstract(float pixels) const {
            return pixels / scaleFactor_;
        }

        // Get the combined scale factor
        float getScaleFactor() const { return scaleFactor_; }
        float getSystemScale() const { return systemScale_; }
        bool isInitialized() const { return initialized_; }

    private:
        Scale() = default;

        void updateScaleFactor() {
            scaleFactor_ = systemScale_ * userScale_;
        }

        float systemScale_ = 1.0f;   // From OS DPI
        float userScale_ = 1.0f;     // User preference
        float scaleFactor_ = 1.0f;   // Combined
        bool initialized_ = false;
    };

} // namespace LibreUI