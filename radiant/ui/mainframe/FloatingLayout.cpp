#include "FloatingLayout.h"

#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"

#include "camera/GlobalCamera.h"
#include "ui/einspector/EntityInspector.h"
#include "ui/texturebrowser/TextureBrowser.h"

namespace ui {

std::string FloatingLayout::getName() {
	return FLOATING_LAYOUT_NAME;
}

void FloatingLayout::activate() {
 	// Get the floating window with the CamWnd packed into it
	gtkutil::PersistentTransientWindowPtr floatingWindow = GlobalCamera().getFloatingWindow();
	GlobalEventManager().connectAccelGroup(GTK_WINDOW(floatingWindow->getWindow()));
  
	floatingWindow->show();

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
}

void FloatingLayout::deactivate() {
	// TODO
}

// The creation function, needed by the mainframe layout manager
FloatingLayoutPtr FloatingLayout::CreateInstance() {
	return FloatingLayoutPtr(new FloatingLayout);
}

} // namespace ui
