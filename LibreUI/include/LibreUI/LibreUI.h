// LibreUI/include/LibreUI/LibreUI.h
// Master include header - include this one file to use LibreUI
//
// Usage:
//   #include <LibreUI/LibreUI.h>
//
// LibreUI is a portable, Vulkan-compatible UI library.
// It provides widgets, theming, and rendering abstractions
// without direct dependencies on Vulkan or windowing systems.

#pragma once

#include <LibreUI/Export.h>

// ============================================================================
// CORE
// ============================================================================
#include <LibreUI/Types.h>      // Vec2, Rect, Color, Padding
#include <LibreUI/Events.h>     // MouseEvent, KeyEvent, callbacks
#include <LibreUI/Scale.h>      // DPI scaling
#include <LibreUI/Theme.h>      // Colors and sizes

// ============================================================================
// BASE CLASSES
// ============================================================================
#include <LibreUI/Widget.h>     // Base widget class
#include <LibreUI/Renderer.h>   // Abstract renderer interface

// ============================================================================
// WIDGETS
// ============================================================================
#include <LibreUI/Widgets/Label.h>
#include <LibreUI/Widgets/Button.h>
#include <LibreUI/Widgets/Panel.h>
#include <LibreUI/Widgets/Menu.h>
#include <LibreUI/Widgets/Window.h>
#include <LibreUI/Widgets/Dropdown.h>

// ============================================================================
// VERSION
// ============================================================================
namespace LibreUI {
    constexpr int VERSION_MAJOR = 0;
    constexpr int VERSION_MINOR = 1;
    constexpr int VERSION_PATCH = 0;
    constexpr const char* VERSION_STRING = "0.1.0";
}