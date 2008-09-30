#ifndef SHADERINFODIALOG_H_
#define SHADERINFODIALOG_H_

#include "map/ShaderBreakdown.h"
#include "gtkutil/window/BlockingTransientWindow.h"

typedef struct _GtkListStore GtkListStore;

namespace ui {

class ShaderInfoDialog :
	public gtkutil::BlockingTransientWindow
{
	// The helper class counting the shaders in the map
	map::ShaderBreakdown _shaderBreakdown;
	
	// The treeview containing the above liststore
	GtkListStore* _listStore;
	GtkWidget* _treeView;
	
public:
	// Constructor
	ShaderInfoDialog();

	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void showDialog();
	
private:
	// This is called to create the widgets
	void populateWindow();

	// Disconnect this window from the eventmanagaer
	void shutdown();

	// Helper method to create the OK/Cancel button
	GtkWidget* createButtons();
	
	// The callback for the buttons
	static void onClose(GtkWidget* widget, ShaderInfoDialog* self);

};

} // namespace ui

#endif /* SHADERINFODIALOG_H_ */
