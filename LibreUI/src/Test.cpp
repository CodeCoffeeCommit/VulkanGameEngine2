// LibreUI/src/Test.cpp
// Temporary test file - verifies all components link correctly
// Delete this file after testing

#include <LibreUI/LibreUI.h>
#include <iostream>

// This function tests that all LibreUI components compile and link
void LibreUI_RunTests() {
    using namespace LibreUI;

    std::cout << "=== LibreUI Test ===" << std::endl;
    std::cout << "Version: " << VERSION_STRING << std::endl;

    // Test Types
    Vec2 v(10, 20);
    Rect r(0, 0, 100, 50);
    Color c(1.0f, 0.5f, 0.0f, 1.0f);
    std::cout << "[OK] Types: Vec2, Rect, Color" << std::endl;

    // Test Scale
    Scale::instance().initialize(1.0f);
    float scaled = Scale::instance().toPixels(10.0f);
    std::cout << "[OK] Scale: 10 units = " << scaled << " pixels" << std::endl;

    // Test Theme
    Theme& theme = GetTheme();
    float fontSize = theme.fontSize();
    std::cout << "[OK] Theme: fontSize = " << fontSize << std::endl;

    // Test Widgets (just construction, no rendering)
    Label label("Test Label");
    Button button("Click Me");
    button.onClick = []() { std::cout << "Button clicked!" << std::endl; };
    Panel panel("Test Panel");
    Dropdown dropdown;
    dropdown.items = { "Option 1", "Option 2", "Option 3" };
    Window window("Test Window");
    MenuBar menuBar;
    menuBar.addMenu("File", {
        MenuItem::Action("New", []() {}),
        MenuItem::Separator(),
        MenuItem::Action("Exit", []() {})
        });

    std::cout << "[OK] Widgets: Label, Button, Panel, Dropdown, Window, MenuBar" << std::endl;

    // Test Events
    MouseEvent mouse;
    mouse.x = 50;
    mouse.y = 25;
    mouse.button = MouseButton::Left;
    mouse.pressed = true;

    KeyEvent key;
    key.key = 256;  // Escape
    key.pressed = true;

    std::cout << "[OK] Events: MouseEvent, KeyEvent" << std::endl;

    std::cout << "=== All Tests Passed! ===" << std::endl;
}