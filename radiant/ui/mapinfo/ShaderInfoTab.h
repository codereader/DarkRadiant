#ifndef SHADERINFOTAB_H_
#define SHADERINFOTAB_H_

#include "map/ShaderBreakdown.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkListStore GtkListStore;

namespace ui {

class ShaderInfoTab
{
	// The "master" widget
	GtkWidget* _widget;

	// The helper class counting the shaders in the map
	map::ShaderBreakdown _shaderBreakdown;
	
	// The treeview containing the above liststore
	GtkListStore* _listStore;
	GtkWidget* _treeView;
	
public:
	// Constructor
	ShaderInfoTab();

	// Use this to pack the tab into a parent container
	GtkWidget* getWidget();

	std::string getLabel();
	std::string getIconName();
	
private:
	// This is called to create the widgets
	void populateTab();

}; // class ShaderInfoTab

} // namespace ui

#endif /* SHADERINFOTAB_H_ */
