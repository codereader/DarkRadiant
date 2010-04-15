#include "PathEntry.h"

#include "iregistry.h"

#include "i18n.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkentry.h>

#include "FramedWidget.h"
#include "IConv.h"
#include "FileChooser.h"

#include "os/path.h"

namespace gtkutil
{

PathEntry::PathEntry(bool foldersOnly)
{
	// path entry
	_entry = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(_entry), FALSE);
	
	// browse button
	GtkButton* button = GTK_BUTTON(gtk_button_new());

	std::string fullFileName = GlobalRegistry().get(RKEY_BITMAPS_PATH) + "ellipsis.png";

	GtkWidget* image = gtk_image_new_from_pixbuf(
		gdk_pixbuf_new_from_file(fullFileName.c_str(), NULL)
	);

	gtk_container_add(GTK_CONTAINER(button), image);

	// Pack entry + button into an hbox
	GtkWidget* hbox = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), _entry, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);

	_topLevel = FramedWidget(hbox);

	// Connect the button
	g_signal_connect(
		G_OBJECT(button), 
		"clicked", 
		G_CALLBACK(foldersOnly ? onBrowseFolders : onBrowseFiles), 
		this
	);
}

void PathEntry::setValue(const std::string& val)
{
	// Convert the value to UTF8 before writing it to the entry box
	std::string utf8Val = gtkutil::IConv::filenameToUTF8(val);

	gtk_entry_set_text(GTK_ENTRY(_entry), utf8Val.c_str());
}

std::string PathEntry::getValue() const
{
	return gtk_entry_get_text(GTK_ENTRY(_entry));
}

GtkWidget* PathEntry::_getWidget() const
{
	return _topLevel;
}

GtkWidget* PathEntry::getEntryWidget() const
{
	return _entry;
}

void PathEntry::onBrowseFiles(GtkWidget* button, PathEntry* self)
{
	FileChooser fileChooser(gtk_widget_get_toplevel(button), _("Choose File"), true, false);

	fileChooser.setCurrentPath(self->getValue());

	std::string filename = fileChooser.display();

	if (GTK_IS_WINDOW(gtk_widget_get_toplevel(button)))
	{
		gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(button)));
	}

	if (!filename.empty())
	{
		self->setValue(gtkutil::IConv::filenameToUTF8(filename));
	}
}

void PathEntry::onBrowseFolders(GtkWidget* button, PathEntry* self)
{
	FileChooser fileChooser(gtk_widget_get_toplevel(button), _("Choose Directory"), true, true);

	std::string curEntry = self->getValue();

	if (!path_is_absolute(curEntry.c_str())) 
	{
		curEntry.clear();
	}

	fileChooser.setCurrentPath(curEntry);

	std::string filename = fileChooser.display();

	if (GTK_IS_WINDOW(gtk_widget_get_toplevel(button)))
	{
		gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(button)));
	}

	if (!filename.empty())
	{
		self->setValue(gtkutil::IConv::filenameToUTF8(filename));
	}
}

} // namespace gtkutil
