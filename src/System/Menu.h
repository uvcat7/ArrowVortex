#include <string>
#include <vector>
#include <Editor/Action.h>

namespace Vortex {

struct MenuItem;

// Helper struct for building the menu bar
struct MenuEntry {
    Action::Type action = Action::Type::NONE;
    std::string item_text = "";
    bool is_separator = false;
    bool is_checked = false;
    bool is_enabled = true;
    MenuItem* submenu = nullptr;
    recti active_rect;
};

struct MenuItem {
    static MenuItem* create();

    void addSeperator();
    void addItem(Action::Type item, const std::string& text);
    void addSubmenu(MenuItem* submenu, const std::string& text,
                    bool grayed = false);
    void replaceSubmenu(int pos, MenuItem* submenu, const std::string& text,
                        bool grayed = false);

    void setChecked(Action::Type item, bool checked);
    void setEnabled(Action::Type item, bool checked);
    void setTopLevel(bool topLevel);
    void setOpen(int pos);
    int getOpen();
    void close();
    std::vector<MenuEntry>& getMenuData();

   private:
    std::vector<MenuEntry> menu_data;
    int open_entry = -1;
    bool is_top_level = false;
};

}  // namespace Vortex