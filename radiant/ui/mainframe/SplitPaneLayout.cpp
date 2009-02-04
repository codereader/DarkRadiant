#include "SplitPaneLayout.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"

#include "gtkutil/FramedWidget.h"

#include "camera/GlobalCamera.h"
#include "ui/einspector/EntityInspector.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

namespace ui {

std::string SplitPaneLayout::getName() {
	return SPLITPANE_LAYOUT_NAME;
}

void SplitPaneLayout::activate() {
 	GtkWidget* page = gtkutil::FramedWidget(
		GlobalTextureBrowser().constructWindow(GTK_WINDOW(GlobalGroupDialog().getDialogWindow()))
	);

	// Add the Texture Browser page to the group dialog
	GlobalGroupDialog().addPage(
    	"textures",	// name
    	"Textures", // tab title
    	"icon_texture.png", // tab icon 
    	GTK_WIDGET(page), // page widget
    	"Texture Browser"
    );

	GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload 
	// the position info from the Registry once again.
	ui::EntityInspector::getInstance().restoreSettings();

	// Restore any floating XYViews that were active before
	// This will create a default view if no saved info is found
	GlobalXYWnd().restoreState();
}

void SplitPaneLayout::deactivate() {
	// Save the current XYViews to the registry
	GlobalXYWnd().saveState();

	// Delete all active views
	GlobalXYWnd().destroyViews();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");
	GlobalTextureBrowser().destroyWindow();
}

// The creation function, needed by the mainframe layout manager
SplitPaneLayoutPtr SplitPaneLayout::CreateInstance() {
	return SplitPaneLayoutPtr(new SplitPaneLayout);
}

} // namespace ui
