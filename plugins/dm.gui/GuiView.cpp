#include "GuiView.h"

#include <gtk/gtkhbox.h>

namespace gui
{

GuiView::GuiView() :
	_glWidget(new gtkutil::GLWidget(true, "GUI")),
	_renderer(*_glWidget)
{
	// Construct the widgets
	_widget = gtk_hbox_new(FALSE, 6);
	gtk_widget_set_size_request(*_glWidget, 640, 480);

	gtk_box_pack_start(GTK_BOX(_widget), *_glWidget, TRUE, TRUE, 0);
}

}
