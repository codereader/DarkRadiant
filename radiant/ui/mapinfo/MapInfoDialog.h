#ifndef MAPINFODIALOG_H_
#define MAPINFODIALOG_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include "EntityInfoTab.h"

typedef struct _GtkNotebook GtkNotebook;

namespace ui {

class MapInfoDialog :
	public gtkutil::BlockingTransientWindow
{
	// The helper class displaying the entity statistics
	EntityInfoTab _entityInfo;
	
	// The tabs of this dialog
	GtkNotebook* _notebook;
	
public:
	// Constructor
	MapInfoDialog();

	/** 
	 * greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void showDialog();
	
private:
	// This is called to create the widgets
	void populateWindow();

	// Disconnect this window from the eventmanagaer
	void shutdown();

	// Helper method to create the OK/Cancel button
	GtkWidget* createButtons();

	GtkWidget* createTabLabel(const std::string& label, const std::string& iconName);
	
	// The callback for the buttons
	static void onClose(GtkWidget* widget, MapInfoDialog* self);

}; // class MapInfoDialog

} // namespace ui

#endif /*MAPINFODIALOG_H_*/
