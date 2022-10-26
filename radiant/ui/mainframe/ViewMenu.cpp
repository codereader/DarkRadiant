#include "ViewMenu.h"

#include "ui/iuserinterface.h"

namespace ui
{

ViewMenu::ViewMenu()
{
    refresh();
}

ViewMenu::~ViewMenu()
{
    GlobalMenuManager().remove("main/window");
}

void ViewMenu::refresh()
{
    // Add view items to menu
    auto& menuManager = GlobalMenuManager();

    menuManager.remove("main/window");
    menuManager.insert("main/help", "window", menu::ItemType::Folder, _("Window"), "", "");

    GlobalUserInterface().foreachControl([&](const std::string& controlName)
    {
        auto control = GlobalUserInterface().findControl(controlName);
        assert(control); // we just got told that this control is present

        auto command = fmt::format("{0}{1}", TOGGLE_CONTROL_STATEMENT_PREFIX, control->getControlName());

        menuManager.add("main/window", command, menu::ItemType::Item, control->getDisplayName(), "",  command);
    });
}

}
