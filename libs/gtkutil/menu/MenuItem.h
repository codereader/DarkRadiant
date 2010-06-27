#ifndef _GTKUTIL_POPUP_MENU_ITEM_H_
#define _GTKUTIL_POPUP_MENU_ITEM_H_

#include "imenu.h"

typedef struct _GtkWidget GtkWidget;

namespace gtkutil
{

// Data class containing the elements of a menu item
class MenuItem :
	public ui::IMenuItem
{
private:
	GtkWidget* _widget;
	ui::IMenu::Callback _callback;
	ui::IMenu::SensitivityTest _sensitivityTest;
	ui::IMenu::VisibilityTest _visibilityTest;
	
public:
	MenuItem(GtkWidget* w, 
			 const ui::IMenu::Callback& c, 
			 const ui::IMenu::SensitivityTest& s,
			 const ui::IMenu::VisibilityTest& v)
	: _widget(w), 
	  _callback(c), 
	  _sensitivityTest(s),
	  _visibilityTest(v)
	{}

	GtkWidget* getWidget()
	{
		return _widget;
	}

	void execute()
	{
		_callback(); 
	}

	bool isVisible()
	{
		return _visibilityTest();
	}

	bool isSensitive()
	{
		return _sensitivityTest();
	}
};
typedef boost::shared_ptr<MenuItem> MenuItemPtr;

} // namespace

#endif /* _GTKUTIL_POPUP_MENU_ITEM_H_ */
