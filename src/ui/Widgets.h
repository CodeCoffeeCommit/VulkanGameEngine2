#pragma once

#include "Core.h"
#include "Theme.h"
#include "UIRenderer.h"
#include <vector>
#include <memory>
#include <string>

namespace libre::ui {

    // ============================================================================
    // BASE WIDGET
    // ============================================================================

    class Widget {
    public:
        virtual ~Widget() = default;

        virtual void layout(const Rect& available);
        virtual void draw(UIRenderer& renderer);
        virtual bool handleMouse(const MouseEvent& event);
        virtual bool handleKey(const KeyEvent& event);

        void addChild(std::unique_ptr<Widget> child);

        Rect bounds;
        bool visible = true;
        bool enabled = true;
        bool hovered = false;

    protected:
        Widget* parent_ = nullptr;
        std::vector<std::unique_ptr<Widget>> children_;
    };

    // ============================================================================
    // LABEL
    // ============================================================================

    class Label : public Widget {
    public:
        Label(const std::string& text = "");

        void draw(UIRenderer& renderer) override;

        std::string text;
        Color color;
        float fontSize = 13.0f;
    };

    // ============================================================================
    // BUTTON
    // ============================================================================

    class Button : public Widget {
    public:
        Button(const std::string& text = "");

        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        std::string text;
        ClickCallback onClick;

        bool pressed = false;
    };

    // ============================================================================
    // PANEL
    // ============================================================================

    class Panel : public Widget {
    public:
        Panel(const std::string& title = "");

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        std::string title;
        bool collapsible = true;
        bool collapsed = false;

    private:
        Rect headerBounds_;
        Rect contentBounds_;
    };

    // ============================================================================
    // DROPDOWN
    // ============================================================================

    class Dropdown : public Widget {
    public:
        Dropdown();

        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        std::vector<std::string> items;
        int selectedIndex = 0;
        IndexCallback onSelect;

        bool open = false;
        int hoveredItem = -1;

    private:
        Rect getDropdownBounds() const;
    };

    // ============================================================================
    // MENU BAR
    // ============================================================================

    class MenuItem {
    public:
        std::string label;
        ClickCallback action;
        std::vector<MenuItem> children;
        bool separator = false;

        MenuItem() = default;
        MenuItem(const std::string& label, ClickCallback action = nullptr)
            : label(label), action(action) {
        }

        static MenuItem Separator() {
            MenuItem m;
            m.separator = true;
            return m;
        }
    };

    class MenuBar : public Widget {
    public:
        MenuBar();

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        void addMenu(const std::string& label, const std::vector<MenuItem>& items);

    private:
        struct Menu {
            std::string label;
            std::vector<MenuItem> items;
            Rect bounds;
        };

        std::vector<Menu> menus_;
        int openMenuIndex_ = -1;
        int hoveredItemIndex_ = -1;

        Rect getMenuDropdownBounds(int menuIndex) const;
    };

    // ============================================================================
    // WINDOW (Floating panel)
    // ============================================================================

    class Window : public Widget {
    public:
        Window(const std::string& title = "");

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        std::string title;
        bool closable = true;
        bool draggable = true;
        bool isOpen = true;

        ClickCallback onClose;

    private:
        Rect titleBarBounds_;
        Rect contentBounds_;
        Rect closeButtonBounds_;

        bool dragging_ = false;
        float dragOffsetX_ = 0;
        float dragOffsetY_ = 0;
        bool closeHovered_ = false;
    };
    // ============================================================================
    // CHECKBOX
    // ============================================================================

    class Checkbox : public Widget {
    public:
        Checkbox(const std::string& label = "");

        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        std::string label;
        bool checked = false;
        std::function<void(bool)> onChange;

    private:
        Rect boxBounds_;
        static constexpr float BOX_SIZE = 16.0f;
    };

    // ============================================================================
    // SLIDER
    // ============================================================================

    class Slider : public Widget {
    public:
        Slider(float minVal = 0.0f, float maxVal = 1.0f);

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        float value = 0.5f;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        std::string label;
        bool showValue = true;
        int precision = 2;  // Decimal places to display

        ValueCallback onChange;

    private:
        float valueToPosition(float val) const;
        float positionToValue(float x) const;

        Rect trackBounds_;
        Rect handleBounds_;
        bool dragging_ = false;
        static constexpr float HANDLE_WIDTH = 12.0f;
        static constexpr float TRACK_HEIGHT = 4.0f;
        static constexpr float LABEL_WIDTH = 80.0f;
        static constexpr float VALUE_WIDTH = 50.0f;
    };

    // ============================================================================
    // TEXT FIELD (Single-line text input)
    // ============================================================================

    class TextField : public Widget {
    public:
        TextField(const std::string& placeholder = "");

        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;
        bool handleKey(const KeyEvent& event) override;

        std::string text;
        std::string placeholder;
        bool password = false;  // Show asterisks instead of text
        int maxLength = 256;

        std::function<void(const std::string&)> onChange;
        std::function<void(const std::string&)> onSubmit;  // Called on Enter

    private:
        bool focused_ = false;
        size_t cursorPos_ = 0;
        float cursorBlinkTime_ = 0.0f;
        bool cursorVisible_ = true;

        // Selection (future enhancement)
        size_t selectionStart_ = 0;
        size_t selectionEnd_ = 0;
    };

    // ============================================================================
    // NUMERIC FIELD (Number input with increment/decrement)
    // ============================================================================

    class NumericField : public Widget {
    public:
        NumericField(float minVal = 0.0f, float maxVal = 100.0f);

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;
        bool handleKey(const KeyEvent& event) override;

        float value = 0.0f;
        float minValue = 0.0f;
        float maxValue = 100.0f;
        float step = 1.0f;
        std::string label;
        int precision = 2;

        ValueCallback onChange;

    private:
        void increment();
        void decrement();
        void clampValue();

        Rect fieldBounds_;
        Rect decrementBounds_;
        Rect incrementBounds_;
        bool editing_ = false;
        std::string editBuffer_;
        bool decrementHovered_ = false;
        bool incrementHovered_ = false;
    };

    // ============================================================================
    // COLOR PICKER (Simple color selection)
    // ============================================================================

    class ColorPicker : public Widget {
    public:
        ColorPicker();

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        Color color{ 1.0f, 1.0f, 1.0f, 1.0f };
        std::string label;

        std::function<void(const Color&)> onChange;

    private:
        Rect previewBounds_;
        Rect expandedBounds_;
        bool expanded_ = false;

        // HSV for picker
        float hue_ = 0.0f;
        float saturation_ = 1.0f;
        float brightness_ = 1.0f;

        Color hsvToRgb(float h, float s, float v);
        void rgbToHsv(const Color& c);
    };

    // ============================================================================
    // SEPARATOR (Visual divider)
    // ============================================================================

    class Separator : public Widget {
    public:
        Separator(bool horizontal = true);

        void draw(UIRenderer& renderer) override;

        bool horizontal = true;
    };

    // ============================================================================
    // SCROLL AREA (Scrollable container)
    // ============================================================================

    class ScrollArea : public Widget {
    public:
        ScrollArea();

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        float scrollOffset = 0.0f;
        bool showScrollbar = true;

    private:
        float contentHeight_ = 0.0f;
        float maxScroll_ = 0.0f;
        Rect scrollbarBounds_;
        Rect thumbBounds_;
        bool scrollbarDragging_ = false;
        float dragStartOffset_ = 0.0f;
    };

    // ============================================================================
    // PROPERTY ROW (Label + Widget combination for settings)
    // ============================================================================

    class PropertyRow : public Widget {
    public:
        PropertyRow(const std::string& label, std::unique_ptr<Widget> control);

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;
        bool handleKey(const KeyEvent& event) override;

        std::string label;
        float labelWidth = 120.0f;

    private:
        std::unique_ptr<Widget> control_;
        Rect labelBounds_;
        Rect controlBounds_;
    };

    // ============================================================================
    // COLLAPSIBLE SECTION (For grouping preferences)
    // ============================================================================

    class CollapsibleSection : public Widget {
    public:
        CollapsibleSection(const std::string& title);

        void layout(const Rect& available) override;
        void draw(UIRenderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;

        std::string title;
        bool collapsed = false;

    private:
        Rect headerBounds_;
        Rect contentBounds_;
        bool headerHovered_ = false;
    };
} // namespace libre::ui