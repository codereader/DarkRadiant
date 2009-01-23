#include "MapPreview.h"

#include "itextstream.h"
#include <gtk/gtk.h>

namespace map {

MapPreview::MapPreview() :
	_previewContainer(gtk_vbox_new(FALSE, 0))
{
	_camera.setSize(400);
	gtk_box_pack_start(GTK_BOX(_previewContainer), _camera, TRUE, TRUE, 0);
}

GtkWidget* MapPreview::getPreviewWidget() {
	gtk_widget_show_all(_previewContainer);
	return _previewContainer;
}

/**
 * Gets called whenever the user changes the file selection.
 * Note: this method must call the setPreviewActive() method on the
 * FileChooser class to indicate whether the widget is active or not.
 */
void MapPreview::onFileSelectionChanged(
	const std::string& newFileName, gtkutil::FileChooser& fileChooser)
{
	// Attempt to load file
	bool success = setMapName(newFileName);

	_camera.initialisePreview();
	gtk_widget_queue_draw(_camera); 
	
	fileChooser.setPreviewActive(success);
}

bool MapPreview::setMapName(const std::string& name) {
	_mapName = name;
	_mapResource = GlobalMapResourceManager().capture(_mapName);

	if (_mapResource == NULL) {
		return false;
	}

	if (_mapResource->load()) {
		// Set the new rootnode
		_camera.setRootNode(_mapResource->getNode());
		return true;
	}
	else {
		// Map load failed
		globalWarningStream() << "Could not load map: " << _mapName << std::endl;
		return false;
	}
}

} // namespace map
