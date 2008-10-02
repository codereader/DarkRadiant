#ifndef MODELINFOTAB_H_
#define MODELINFOTAB_H_

#include "map/ModelBreakdown.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkListStore GtkListStore;

namespace ui {

class ModelInfoTab
{
	// The "master" widget
	GtkWidget* _widget;

	// The helper class counting the models in the map
	map::ModelBreakdown _modelBreakdown;
	
	// The treeview containing the above liststore
	GtkListStore* _listStore;
	GtkWidget* _treeView;
	
public:
	// Constructor
	ModelInfoTab();

	// Use this to pack the tab into a parent container
	GtkWidget* getWidget();

	std::string getLabel();
	std::string getIconName();
	
private:
	// This is called to create the widgets
	void populateTab();

}; // class ModelInfoTab

} // namespace ui

#endif /* MODELINFOTAB_H_ */
