// LibreUI/include/LibreUI/Events.h
// Input events and callbacks for LibreUI - no platform dependencies

#pragma once

#include <functional>

namespace LibreUI {

    // ============================================================================
    // INPUT ENUMS
    // ============================================================================

    enum class MouseButton {
        Left,
        Right,
        Middle
    };

    // Key codes - platform layer translates to these
    // Using GLFW values for now, but apps translate to these
    enum class Key {
        Unknown = -1,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        Home = 268,
        End = 269
        // Add more as needed
    };

    // ============================================================================
    // INPUT EVENTS
    // ============================================================================

    struct MouseEvent {
        float x = 0, y = 0;
        MouseButton button = MouseButton::Left;
        bool pressed = false;
        bool released = false;
        float scroll = 0;
    };

    struct KeyEvent {
        int key = 0;
        bool pressed = false;
        bool shift = false;
        bool ctrl = false;
        bool alt = false;
    };

    // ============================================================================
    // CALLBACKS
    // ============================================================================

    using ClickCallback = std::function<void()>;
    using ValueCallback = std::function<void(float)>;
    using IndexCallback = std::function<void(int)>;
    using TextCallback = std::function<void(const std::string&)>;

} // namespace LibreUI