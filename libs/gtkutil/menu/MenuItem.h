#pragma once

#include "imenu.h"

#include <wx/menuitem.h>

namespace wxutil
{

// Data class containing the elements of a menu item
class MenuItem :
	public ui::IMenuItem
{
protected:
	wxMenuItem* _menuItem;
	ui::IMenu::Callback _callback;
	ui::IMenu::SensitivityTest _sensitivityTest;
	ui::IMenu::VisibilityTest _visibilityTest;

public:
	MenuItem(wxMenuItem* menuItem,
			 const ui::IMenu::Callback& callback,
			 const ui::IMenu::SensitivityTest& sensTest = AlwaysSensitive,
			 const ui::IMenu::VisibilityTest& visTest = AlwaysVisible)
	: _menuItem(menuItem),
	  _callback(callback),
	  _sensitivityTest(sensTest),
	  _visibilityTest(visTest)
	{}

	virtual wxMenuItem* getMenuItem()
	{
		return _menuItem;
	}

	virtual void execute()
	{
		_callback();
	}

	virtual bool isVisible()
	{
		return _visibilityTest();
	}

	virtual bool isSensitive()
	{
		return _sensitivityTest();
	}

	static bool AlwaysVisible() { return true; }
	static bool AlwaysSensitive() { return true; }

	void onActivate()
	{
		execute();
	}
};
typedef std::shared_ptr<MenuItem> MenuItemPtr;

} // namespace
