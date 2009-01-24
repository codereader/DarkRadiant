#include "MapFileChooserPreview.h"

#include "itextstream.h"
#include <gtk/gtkvbox.h>
#include "brush/BrushModule.h"
#include "selection/algorithm/Primitives.h"

namespace map {

MapFileChooserPreview::MapFileChooserPreview() :
	_previewContainer(gtk_vbox_new(FALSE, 0))
{
	_preview.setSize(400);
	gtk_box_pack_start(GTK_BOX(_previewContainer), _preview, TRUE, TRUE, 0);
}

GtkWidget* MapFileChooserPreview::getPreviewWidget() {
	gtk_widget_show_all(_previewContainer);
	return _previewContainer;
}

/**
 * Gets called whenever the user changes the file selection.
 * Note: this method must call the setPreviewActive() method on the
 * FileChooser class to indicate whether the widget is active or not.
 */
void MapFileChooserPreview::onFileSelectionChanged(
	const std::string& newFileName, gtkutil::FileChooser& fileChooser)
{
	// Attempt to load file
	bool success = setMapName(newFileName);

	_preview.initialisePreview();
	gtk_widget_queue_draw(_preview); 
	
	fileChooser.setPreviewActive(success);
}

bool MapFileChooserPreview::setMapName(const std::string& name) {
	_mapName = name;
	_mapResource = GlobalMapResourceManager().capture(_mapName);

	if (_mapResource == NULL) {
		return false;
	}

	if (_mapResource->load()) {
		// Get the node from the reosource
		scene::INodePtr root = _mapResource->getNode();

		assert(root != NULL);

		// Treat func_statics
		selection::algorithm::addOriginToChildPrimitives(root);

		// Set the new rootnode
		_preview.setRootNode(root);

		return true;
	}
	else {
		// Map load failed
		globalWarningStream() << "Could not load map: " << _mapName << std::endl;
		return false;
	}
}

} // namespace map
