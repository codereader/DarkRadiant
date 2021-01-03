#pragma once

#include "imenu.h"

#include <wx/menuitem.h>

namespace wxutil
{

// Represents a separator item
class SeparatorItem final :
    public ui::IMenuItem
{
protected:
    wxMenuItem* _menuItem;
    ui::IMenu::VisibilityTest _visibilityTest;

public:
    SeparatorItem(wxMenuItem* menuItem, const ui::IMenu::VisibilityTest& visTest = AlwaysVisible) : 
        _menuItem(menuItem),
        _visibilityTest(visTest)
    {}

    wxMenuItem* getMenuItem() override
    {
        return _menuItem;
    }

    void execute() override
    {}

    bool isVisible() override
    {
        return _visibilityTest();
    }

    static bool AlwaysVisible() { return true; }
};

} // namespace
