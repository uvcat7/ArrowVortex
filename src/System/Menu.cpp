#include <System/Menu.h>
#include <vector>

namespace Vortex {

#ifdef _WIN32
MenuItem* MenuItem::create() {
    return reinterpret_cast<MenuItem*>(CreatePopupMenu());
}

void MenuItem::addSeperator() {
    AppendMenuW(reinterpret_cast<HMENU>(this), MF_SEPARATOR, 0, nullptr);
}
void MenuItem::addItem(int item, const std::string& text) {
    AppendMenuW(reinterpret_cast<HMENU>(this), MF_STRING, item,
                Widen(text).c_str());
}

void MenuItem::addSubmenu(MenuItem* submenu, const std::string& text,
                          bool grayed) {
    int flags = MF_STRING | MF_POPUP | (grayed * MF_GRAYED);
    AppendMenuW(reinterpret_cast<HMENU>(this), MF_STRING | MF_POPUP,
                reinterpret_cast<UINT_PTR>(submenu), Widen(text).c_str());
}

void MenuItem::replaceSubmenu(int pos, MenuItem* submenu,
                              const std::string& text, bool grayed) {
    int flags = MF_BYPOSITION | MF_STRING | MF_POPUP | (grayed * MF_GRAYED);
    DeleteMenu(reinterpret_cast<HMENU>(this), pos, MF_BYPOSITION);
    InsertMenuW(reinterpret_cast<HMENU>(this), pos, flags,
                reinterpret_cast<UINT_PTR>(submenu), Widen(text).c_str());
}

void MenuItem::setChecked(int item, bool state) {
    CheckMenuItem(reinterpret_cast<HMENU>(this), item,
                  state ? MF_CHECKED : MF_UNCHECKED);
}

void MenuItem::setEnabled(int item, bool state) {
    EnableMenuItem(reinterpret_cast<HMENU>(this), item,
                   state ? MF_ENABLED : MF_GRAYED);
}
#else
MenuItem* MenuItem::create() { return new MenuItem(); }

void MenuItem::addSeperator() {
    menu_data.push_back(
        {Action::Type::NONE, "", true, false, true, nullptr, false});
}

void MenuItem::addItem(Action::Type item, const std::string& text) {
    menu_data.push_back({item, text, false, false, true, nullptr, false});
}

void MenuItem::addSubmenu(MenuItem* submenu, const std::string& text,
                          bool grayed) {
    menu_data.push_back(
        {Action::Type::NONE, text, false, false, grayed, submenu, false});
}

void MenuItem::replaceSubmenu(int pos, MenuItem* submenu,
                              const std::string& text, bool grayed) {
    MenuEntry item = {
        Action::Type::NONE, text, false, false, grayed, submenu, false};
    std::swap(menu_data[pos], item);
}

void MenuItem::setChecked(Action::Type item, bool state) {
    for (auto it : menu_data) {
        if (it.action == item) {
            it.is_checked = state;
        }
    }
}

void MenuItem::setEnabled(Action::Type item, bool state) {
    for (auto it : menu_data) {
        if (it.action == item) {
            it.is_enabled = state;
        }
    }
}

std::vector<MenuEntry>& MenuItem::getMenuData() {
    return menu_data;
}

void MenuItem::setTopLevel(bool topLevel) { is_top_level = topLevel; }

MenuItem* MenuItem::setOpen(int pos, bool open) {
    menu_data[pos].is_open = open;
    return menu_data[pos].submenu;
}

#endif

}  // namespace Vortex