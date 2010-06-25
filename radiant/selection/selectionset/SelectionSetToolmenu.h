#ifndef _SELECTION_SET_TOOL_MENU_H_
#define _SELECTION_SET_TOOL_MENU_H_

#include <boost/shared_ptr.hpp>

typedef struct _GtkToolItem GtkToolItem;
typedef struct _GtkWidget GtkWidget;

namespace selection
{

class SelectionSetToolmenu
{
private:
	GtkToolItem* _toolItem;

	GtkWidget* _entry;

public:
	SelectionSetToolmenu();

	// Get the tool item widget for packing this control into a GtkToolbar
	GtkToolItem* getToolItem();
};
typedef boost::shared_ptr<SelectionSetToolmenu> SelectionSetToolmenuPtr;

} // namespace selection

#endif /* _SELECTION_SET_TOOL_MENU_H_ */
