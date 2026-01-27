LibreUI Include Syntax

Single include (recommended):

cpp

     #include <LibreUI/LibreUI.h>

This gives you everything - all types, widgets, theme, events.

Individual includes (if you want granular control):

cpp

    #include <LibreUI/Types.h> 
    #include <LibreUI/Events.h>
    #include <LibreUI/Theme.h>
    #include <LibreUI/Widget.h>
    #include <LibreUI/Renderer.h>
    #include <LibreUI/Widgets/Button.h>
    #include <LibreUI/Widgets/Panel.h>
    #include <LibreUI/Widgets/Menu.h>


Usage example:

cpp

    #include <LibreUI/LibreUI.h>
    void createUI() {    
    LibreUI::Button btn("Click Me");
    btn.onClick = []() { 
    std::cout << "Clicked!\n"; 
    };
    LibreUI::Panel panel("Settings");    
    panel.addChild(
    std::make_unique<LibreUI::Label>("Hello"));
    LibreUI::Theme& theme = LibreUI::GetTheme();    
    theme.accent = LibreUI::Color(1.0f, 0.5f, 0.0f);
    }
