#include "ReadableEditorDialog.h"

#include "imainframe.h"
#include "gtkutil/MultiMonitor.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkbutton.h>

#include "string/string.h"


namespace ui
{

namespace 
{
	const std::string WINDOW_TITLE("Readable Editor");
}

ReadableEditorDialog::ReadableEditorDialog() :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
	_guiView(new gui::GuiView)
{
	// Set size of the window, default size is too narrow for path entries
	GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(
		GlobalMainFrame().getTopLevelWindow()
	);

	gtk_window_set_default_size(GTK_WINDOW(getWindow()), static_cast<gint>(rect.width*0.4f), -1);

	// Add a vbox for the dialog elements
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), _guiView->getWidget(), TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
}

void ReadableEditorDialog::RunDialog(const cmd::ArgumentList& args)
{
	ReadableEditorDialog dialog;
	dialog.show();
}

} // namespace ui
