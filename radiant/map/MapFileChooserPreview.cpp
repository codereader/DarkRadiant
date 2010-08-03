#include "MapFileChooserPreview.h"

#include "imap.h"
#include "i18n.h"
#include "itextstream.h"
#include <gtk/gtkvbox.h>
#include "scenelib.h"
#include <gtk/gtktextview.h>
#include "brush/BrushModule.h"
#include "gtkutil/ScrolledFrame.h"
#include "selection/algorithm/Primitives.h"
#include "map/algorithm/WorldspawnArgFinder.h"

namespace map {

MapFileChooserPreview::MapFileChooserPreview() :
	_previewContainer(gtk_vbox_new(FALSE, 0))
{
	_preview.setSize(400);

	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(_preview.gobj()), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), createUsagePanel(), TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(_previewContainer), vbox, TRUE, TRUE, 0);
}

// Create the entity usage information panel
GtkWidget* MapFileChooserPreview::createUsagePanel() {
	// Create a GtkTextView
	_usageInfo = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_usageInfo), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(_usageInfo), FALSE);

	return gtkutil::ScrolledFrame(_usageInfo);	
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
	const std::string& newFileName, ui::IFileChooser& fileChooser)
{
	// Attempt to load file
	/*bool success = */setMapName(newFileName);

	_preview.initialisePreview();
	gtk_widget_queue_draw(GTK_WIDGET(_preview.gobj()));
	
	// Always have the preview active
	fileChooser.setPreviewActive(true);

	updateUsageInfo();
}

void MapFileChooserPreview::updateUsageInfo() {
	// Get the underlying buffer object	
	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_usageInfo));

	std::string usage("");

	if (_mapResource != NULL && _mapResource->getNode() != NULL) {
		// Retrieve the root node
		scene::INodePtr root = _mapResource->getNode();

		// Traverse the root to find the worldspawn
		WorldspawnArgFinder finder("editor_description");
		Node_traverseSubgraph(root, finder);

		usage = finder.getFoundValue();

		if (usage.empty()) {
			usage = _("<no description>");
		}
	}

	gtk_text_buffer_set_text(buf, usage.c_str(), -1);
}

bool MapFileChooserPreview::setMapName(const std::string& name) {
	_mapName = name;
	_mapResource = GlobalMapResourceManager().capture(_mapName);

	bool success = false;

	if (_mapResource == NULL) {
		// NULLify the preview map root on failure
		_preview.setRootNode(scene::INodePtr());
		return success;
	}

	// Suppress the map loading dialog to avoid user 
	// getting stuck in the "drag filename" operation
	std::string prevValue = GlobalRegistry().get(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG);
	GlobalRegistry().set(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG, "1");

	if (_mapResource->load()) {
		// Get the node from the reosource
		scene::INodePtr root = _mapResource->getNode();

		assert(root != NULL);

		// Set the new rootnode
		_preview.setRootNode(root);

		success = true;
	}
	else {
		// Map load failed
		globalWarningStream() << "Could not load map: " << _mapName << std::endl;
	}

	GlobalRegistry().set(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG, prevValue);

	return success;
}

} // namespace map
