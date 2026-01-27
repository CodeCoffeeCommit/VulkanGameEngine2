// LibreUI/include/LibreUI/Widgets/Menu.h
// Menu bar and dropdown menu system

#pragma once

#include <LibreUI/Widget.h>
#include <LibreUI/Theme.h>
#include <vector>
#include <LibreUI/Export.h>

namespace LibreUI {

    // ============================================================================
    // MENU ITEM
    // ============================================================================

    class LIBREUI_API MenuItem {
    public:
        std::string label;
        std::string shortcut;           // "Ctrl+Z", "F3", etc.
        std::string icon;               // Icon name (future use)
        ClickCallback action;
        std::vector<MenuItem> children; // For submenus

        bool separator = false;
        bool enabled = true;
        bool checkable = false;
        bool checked = false;
        bool* checkedPtr = nullptr;     // Pointer to external bool

        MenuItem() = default;

        MenuItem(const std::string& label, ClickCallback action = nullptr,
            const std::string& shortcut = "")
            : label(label), shortcut(shortcut), action(action) {
        }

        // Static factory methods
        static MenuItem Separator() {
            MenuItem m;
            m.separator = true;
            return m;
        }

        static MenuItem Action(const std::string& label, ClickCallback action,
            const std::string& shortcut = "") {
            MenuItem m;
            m.label = label;
            m.action = action;
            m.shortcut = shortcut;
            return m;
        }

        static MenuItem Toggle(const std::string& label, bool* value,
            const std::string& shortcut = "") {
            MenuItem m;
            m.label = label;
            m.shortcut = shortcut;
            m.checkable = true;
            m.checkedPtr = value;
            m.action = [value]() {
                if (value) *value = !(*value);
                };
            return m;
        }

        static MenuItem Submenu(const std::string& label, std::vector<MenuItem> children) {
            MenuItem m;
            m.label = label;
            m.children = std::move(children);
            return m;
        }

        bool hasSubmenu() const { return !children.empty(); }

        bool isChecked() const {
            return checkedPtr ? *checkedPtr : checked;
        }
    };

    // ============================================================================
    // MENU BAR
    // ============================================================================

    class LIBREUI_API MenuBar : public Widget {
    public:
        MenuBar();

        void layout(const Rect& available) override;
        void draw(Renderer& renderer) override;
        bool handleMouse(const MouseEvent& event) override;
        bool handleKey(const KeyEvent& event) override;

        void addMenu(const std::string& label, const std::vector<MenuItem>& items);

        void closeDropdown() { openMenuIndex_ = -1; hoveredItemIndex_ = -1; }
        bool isDropdownOpen() const { return openMenuIndex_ >= 0; }

    private:
        struct Menu {
            std::string label;
            std::vector<MenuItem> items;
            Rect bounds;
            bool hovered = false;
        };

        std::vector<Menu> menus_;
        int openMenuIndex_ = -1;
        int hoveredItemIndex_ = -1;

        // Layout constants
        static constexpr float DROPDOWN_PADDING = 4.0f;
        static constexpr float ICON_WIDTH = 20.0f;
        static constexpr float SHORTCUT_MIN_GAP = 20.0f;
        static constexpr float CHECKBOX_WIDTH = 18.0f;
        static constexpr float SUBMENU_ARROW_WIDTH = 16.0f;
        static constexpr float MIN_DROPDOWN_WIDTH = 150.0f;
        static constexpr float SEPARATOR_HEIGHT = 7.0f;

        Rect dropdownBounds_;

        // Helper methods
        Rect calculateDropdownBounds(int menuIndex, Renderer& renderer);
        void drawDropdown(Renderer& renderer, int menuIndex);
        void drawMenuItem(Renderer& renderer, const MenuItem& item, const Rect& itemBounds,
            bool hovered, bool enabled);
        float calculateDropdownWidth(const Menu& menu, Renderer& renderer);
    };

} // namespace LibreUI