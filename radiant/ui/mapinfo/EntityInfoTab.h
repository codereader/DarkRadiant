#ifndef ENTITYINFOTAB_H_
#define ENTITYINFOTAB_H_

#include "map/EntityBreakdown.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkListStore GtkListStore;

namespace ui {

class EntityInfoTab
{
	// The "master" widget
	GtkWidget* _widget;

	// The helper class counting the entities in the map
	map::EntityBreakdown _entityBreakdown;
	
	GtkWidget* _brushCount;
	GtkWidget* _patchCount;
	GtkWidget* _entityCount;
	
	// The treeview containing the above liststore
	GtkListStore* _listStore;
	GtkWidget* _treeView;
	
public:
	// Constructor
	EntityInfoTab();

	// Use this to pack the tab into a parent container
	GtkWidget* getWidget();

	std::string getLabel();
	std::string getIconName();
	
private:
	// This is called to create the widgets
	void populateTab();

}; // class EntityInfoTab

} // namespace ui

#endif /* ENTITYINFOTAB_H_ */
