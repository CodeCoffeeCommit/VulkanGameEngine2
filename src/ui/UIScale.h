// src/ui/UIScale.h
// Blender-style DPI-aware UI scaling system
// 
// This singleton manages the global scale factor for all UI elements.
// Scale factor is calculated once at startup based on monitor DPI and
// stays constant during window resize (Blender behavior).
//
// Usage:
//   UIScale::instance().initialize(window);  // Call once at startup
//   float pixels = UIScale::instance().toPixels(abstractUnits);

#pragma once

#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

namespace libre::ui {

    class UIScale {
    public:
        // Singleton access
        static UIScale& instance() {
            static UIScale inst;
            return inst;
        }

        // ====================================================================
        // INITIALIZATION
        // ====================================================================

        // Call ONCE at startup to detect system DPI and set base scale
        void initialize(GLFWwindow* window) {
            if (initialized_) return;

            // Get system DPI scale from GLFW
            // On Windows: 1.0 = 100%, 1.25 = 125%, 1.5 = 150%, 2.0 = 200%
            float xscale, yscale;
            glfwGetWindowContentScale(window, &xscale, &yscale);
            systemScale_ = std::max(xscale, yscale);

            updateScaleFactor();
            initialized_ = true;

            std::cout << "[UIScale] Initialized\n"
                << "  System DPI scale: " << systemScale_ << "\n"
                << "  User scale: " << userScale_ << "\n"
                << "  Combined scale factor: " << scaleFactor_ << "\n";
        }

        // Call when window moves to different monitor (multi-monitor support)
        void onMonitorChanged(GLFWwindow* window) {
            float xscale, yscale;
            glfwGetWindowContentScale(window, &xscale, &yscale);
            float newScale = std::max(xscale, yscale);

            if (std::abs(newScale - systemScale_) > 0.001f) {
                systemScale_ = newScale;
                updateScaleFactor();
                fontsNeedReload_ = true;

                std::cout << "[UIScale] Monitor changed\n"
                    << "  New system scale: " << systemScale_ << "\n"
                    << "  New combined scale: " << scaleFactor_ << "\n";
            }
        }

        // ====================================================================
        // USER PREFERENCE
        // ====================================================================

        // Set user preference scale (like Blender's "Resolution Scale" slider)
        // Range: 0.5 to 3.0 (50% to 300%)
        void setUserScale(float scale) {
            userScale_ = std::clamp(scale, 0.5f, 3.0f);
            updateScaleFactor();
            fontsNeedReload_ = true;

            std::cout << "[UIScale] User scale changed to " << userScale_
                << " (combined: " << scaleFactor_ << ")\n";
        }

        float getUserScale() const { return userScale_; }

        // ====================================================================
        // SCALE FACTOR ACCESS
        // ====================================================================

        // Get the combined scale factor (systemScale * userScale)
        float getScaleFactor() const { return scaleFactor_; }

        // Get just the system-detected scale
        float getSystemScale() const { return systemScale_; }

        // Check if scaling system has been initialized
        bool isInitialized() const { return initialized_; }

        // ====================================================================
        // UNIT CONVERSION
        // ====================================================================

        // Convert abstract design units to actual pixels
        // Example: toPixels(22.0f) with 2.0 scale = 44 pixels
        float toPixels(float units) const {
            return units * scaleFactor_;
        }

        // Convert actual pixels back to abstract units
        // Example: toUnits(44.0f) with 2.0 scale = 22 units
        float toUnits(float pixels) const {
            return pixels / scaleFactor_;
        }

        // Get font pixel size from point size
        // Rounds to nearest integer for crisp text rendering
        int getFontPixelSize(float pointSize) const {
            return static_cast<int>(pointSize * scaleFactor_ + 0.5f);
        }

        // ====================================================================
        // FONT RELOAD TRACKING
        // ====================================================================

        // Check if fonts need reloading after scale change
        bool fontsNeedReload() const { return fontsNeedReload_; }

        // Clear the flag after fonts have been reloaded
        void clearFontsNeedReload() { fontsNeedReload_ = false; }

    private:
        UIScale() = default;
        UIScale(const UIScale&) = delete;
        UIScale& operator=(const UIScale&) = delete;

        void updateScaleFactor() {
            scaleFactor_ = systemScale_ * userScale_;
        }

        bool initialized_ = false;
        float systemScale_ = 1.0f;    // From OS (1.0, 1.25, 1.5, 2.0, etc.)
        float userScale_ = 1.0f;       // User preference (default 1.0)
        float scaleFactor_ = 1.0f;     // Combined: systemScale_ * userScale_
        bool fontsNeedReload_ = false;
    };

} // namespace libre::ui