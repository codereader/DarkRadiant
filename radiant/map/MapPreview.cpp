#include "MapPreview.h"

#include "itextstream.h"
#include <gtk/gtk.h>

namespace map {

MapPreview::MapPreview()
{}

GtkWidget* MapPreview::getPreviewWidget() {
	return gtk_vbox_new(FALSE, 0);;
}

/**
 * Gets called whenever the user changes the file selection.
 * Note: this method must call the setPreviewActive() method on the
 * FileChooser class to indicate whether the widget is active or not.
 */
void MapPreview::onFileSelectionChanged(
	const std::string& newFileName, gtkutil::FileChooser& fileChooser)
{
	fileChooser.setPreviewActive(false);

	globalOutputStream() << "Selection changed to " << newFileName << std::endl;
}

} // namespace map
