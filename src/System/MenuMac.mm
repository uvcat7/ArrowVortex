#import <Cocoa/Cocoa.h>

#include <System/Menu.h>

#include <Editor/Action.h>
#include <Editor/Editor.h>

@interface ArrowVortexMenuTarget : NSObject
- (void)performAction:(id)sender;
@end

@implementation ArrowVortexMenuTarget
- (void)performAction:(id)sender {
    if (Vortex::gEditor)
        Vortex::gEditor->onMenuAction(static_cast<int>([sender tag]));
}
@end

namespace Vortex {
namespace {

ArrowVortexMenuTarget* MenuTarget() {
    static ArrowVortexMenuTarget* target = [[ArrowVortexMenuTarget alloc] init];
    return target;
}

NSString* ToNSString(const std::string& value) {
    return [NSString stringWithUTF8String:value.c_str()];
}

NSMenuItem* FindActionItem(NSMenu* menu, NSInteger action) {
    for (NSMenuItem* item in [menu itemArray]) {
        if ([item tag] == action && action != Action::NONE) return item;
        if ([item hasSubmenu]) {
            if (NSMenuItem* match = FindActionItem([item submenu], action))
                return match;
        }
    }
    return nil;
}

void ApplyShortcut(NSMenuItem* item, NSString* notation) {
    if (!notation || [notation length] == 0) return;

    NSEventModifierFlags modifiers = 0;
    NSString* key = nil;
    for (NSString* rawPart in [notation componentsSeparatedByString:@"+"]) {
        NSString* part = [[rawPart
            stringByTrimmingCharactersInSet:[NSCharacterSet
                                                whitespaceCharacterSet]]
            lowercaseString];
        if ([part isEqualToString:@"ctrl"] ||
            [part isEqualToString:@"control"] ||
            [part isEqualToString:@"cmd"] ||
            [part isEqualToString:@"command"]) {
            modifiers |= NSEventModifierFlagCommand;
        } else if ([part isEqualToString:@"shift"]) {
            modifiers |= NSEventModifierFlagShift;
        } else if ([part isEqualToString:@"alt"] ||
                   [part isEqualToString:@"option"]) {
            modifiers |= NSEventModifierFlagOption;
        } else {
            key = part;
        }
    }
    if (!key || [key length] == 0) return;
    if ([key isEqualToString:@"delete"])
        key = [NSString
            stringWithFormat:@"%C", static_cast<unichar>(NSDeleteCharacter)];
    else if ([key isEqualToString:@"space"])
        key = @" ";
    else if ([key isEqualToString:@"return"])
        key = @"\r";
    else if ([key length] > 1)
        return;

    [item setKeyEquivalent:key];
    [item setKeyEquivalentModifierMask:modifiers];
}

NSMenuItem* CreateActionItem(Action::Type action, const std::string& text) {
    const size_t separator = text.find('\t');
    const std::string title = text.substr(0, separator);
    NSString* shortcut = separator == std::string::npos
                             ? nil
                             : ToNSString(text.substr(separator + 1));
    NSMenuItem* item =
        [[NSMenuItem alloc] initWithTitle:ToNSString(title)
                                   action:@selector(performAction:)
                            keyEquivalent:@""];
    [item setTarget:MenuTarget()];
    [item setTag:static_cast<NSInteger>(action)];
    ApplyShortcut(item, shortcut);
    return item;
}

void AddApplicationMenu(NSMenu* mainMenu) {
    NSMenu* applicationMenu = [[NSMenu alloc] initWithTitle:@"ArrowVortex"];
    [applicationMenu
        addItem:CreateActionItem(Action::SHOW_ABOUT, "About ArrowVortex")];
    [applicationMenu addItem:[NSMenuItem separatorItem]];
    [applicationMenu addItem:CreateActionItem(Action::EXIT_PROGRAM,
                                              "Quit ArrowVortex\tCtrl+Q")];

    NSMenuItem* applicationItem =
        [[NSMenuItem alloc] initWithTitle:@"ArrowVortex"
                                   action:nil
                            keyEquivalent:@""];
    [applicationItem setSubmenu:applicationMenu];
    [mainMenu addItem:applicationItem];
}

}  // namespace

MenuItem* MenuItem::create() {
    static bool createdMainMenu = false;
    MenuItem* item = new MenuItem();
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@""];
    item->native_menu = (__bridge_retained void*)menu;
    if (!createdMainMenu) {
        createdMainMenu = true;
        AddApplicationMenu(menu);
        [NSApp setMainMenu:menu];
    }
    return item;
}

void MenuItem::addSeperator() {
    [(__bridge NSMenu*)native_menu addItem:[NSMenuItem separatorItem]];
}

void MenuItem::addItem(Action::Type action, const std::string& text) {
    [(__bridge NSMenu*)native_menu addItem:CreateActionItem(action, text)];
}

void MenuItem::addSubmenu(MenuItem* submenu, const std::string& text,
                          bool grayed) {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:ToNSString(text)
                                                  action:nil
                                           keyEquivalent:@""];
    [item setSubmenu:(__bridge NSMenu*)submenu->native_menu];
    [item setEnabled:!grayed];
    [(__bridge NSMenu*)native_menu addItem:item];
}

void MenuItem::replaceSubmenu(int pos, MenuItem* submenu,
                              const std::string& text, bool grayed) {
    NSMenu* menu = (__bridge NSMenu*)native_menu;
    if (pos < 0 || pos >= [menu numberOfItems]) return;
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:ToNSString(text)
                                                  action:nil
                                           keyEquivalent:@""];
    [item setSubmenu:(__bridge NSMenu*)submenu->native_menu];
    [item setEnabled:!grayed];
    [menu removeItemAtIndex:pos];
    [menu insertItem:item atIndex:pos];
}

void MenuItem::setChecked(Action::Type action, bool checked) {
    NSMenuItem* item = FindActionItem((__bridge NSMenu*)native_menu, action);
    if (item)
        [item
            setState:checked ? NSControlStateValueOn : NSControlStateValueOff];
}

void MenuItem::setEnabled(Action::Type action, bool enabled) {
    NSMenuItem* item = FindActionItem((__bridge NSMenu*)native_menu, action);
    if (item) [item setEnabled:enabled];
}

void MenuItem::setTopLevel(bool) {}
void MenuItem::setOpen(int) {}
int MenuItem::getOpen() { return -1; }
void MenuItem::close() { [(__bridge NSMenu*)native_menu cancelTracking]; }
std::vector<MenuEntry>& MenuItem::getMenuData() { return menu_data; }

}  // namespace Vortex
