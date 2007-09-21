#ifndef MAPINFODIALOG_H_
#define MAPINFODIALOG_H_

#include "gtkutil/DialogWindow.h"
#include "map/EntityBreakdown.h"

typedef struct _GtkListStore GtkListStore;

namespace ui {

class MapInfoDialog :
	public gtkutil::DialogWindow
{
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
	MapInfoDialog();
	~MapInfoDialog();
	
	// This is called to initialise the dialog window / create the widgets
	virtual void populateWindow();
	
	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void showDialog();
	
private:

	// Helper method to create the OK/Cancel button
	GtkWidget* createButtons();
	
	// The callback for the buttons
	static void onClose(GtkWidget* widget, MapInfoDialog* self);

}; // class MapInfoDialog

} // namespace ui

#endif /*MAPINFODIALOG_H_*/
