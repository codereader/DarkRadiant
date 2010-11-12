#ifndef _GTKUTIL_MENU_ITEM_H_
#define _GTKUTIL_MENU_ITEM_H_

#include "imenu.h"

#include <gtkmm/menuitem.h>

namespace gtkutil
{

// Data class containing the elements of a menu item
class MenuItem :
	public ui::IMenuItem
{
protected:
	Gtk::MenuItem* _widget;
	ui::IMenu::Callback _callback;
	ui::IMenu::SensitivityTest _sensitivityTest;
	ui::IMenu::VisibilityTest _visibilityTest;

	sigc::connection _conn;

public:
	MenuItem(Gtk::MenuItem* w,
			 const ui::IMenu::Callback& c,
			 const ui::IMenu::SensitivityTest& s = AlwaysSensitive,
			 const ui::IMenu::VisibilityTest& v = AlwaysVisible)
	: _widget(w),
	  _callback(c),
	  _sensitivityTest(s),
	  _visibilityTest(v)
	{
		// Connect up the activation callback
		_conn = _widget->signal_activate().connect(sigc::mem_fun(*this, &MenuItem::onActivate));
	}

	~MenuItem()
	{
		_conn.disconnect();
	}

	virtual Gtk::MenuItem* getWidget()
	{
		return _widget;
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
typedef boost::shared_ptr<MenuItem> MenuItemPtr;

} // namespace

#endif /* _GTKUTIL_MENU_ITEM_H_ */
