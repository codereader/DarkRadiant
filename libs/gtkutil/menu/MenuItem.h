#ifndef _GTKUTIL_MENU_ITEM_H_
#define _GTKUTIL_MENU_ITEM_H_

#include "imenu.h"

#include <gtk/gtkwidget.h>

typedef struct _GtkMenuItem GtkMenuItem;

namespace gtkutil
{

// Data class containing the elements of a menu item
class MenuItem :
	public ui::IMenuItem
{
protected:
	GtkWidget* _widget;
	ui::IMenu::Callback _callback;
	ui::IMenu::SensitivityTest _sensitivityTest;
	ui::IMenu::VisibilityTest _visibilityTest;

public:
	MenuItem(GtkWidget* w, 
			 const ui::IMenu::Callback& c, 
			 const ui::IMenu::SensitivityTest& s = AlwaysSensitive,
			 const ui::IMenu::VisibilityTest& v = AlwaysVisible)
	: _widget(w), 
	  _callback(c), 
	  _sensitivityTest(s),
	  _visibilityTest(v)
	{
		// Connect up the activation callback to GTK.
		g_signal_connect(G_OBJECT(_widget), "activate", G_CALLBACK(onActivate),	this);
	}

	virtual GtkWidget* getWidget()
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

	static void onActivate(GtkMenuItem* item, MenuItem* self)
	{
		self->execute();
	}
};
typedef boost::shared_ptr<MenuItem> MenuItemPtr;

} // namespace

#endif /* _GTKUTIL_MENU_ITEM_H_ */
