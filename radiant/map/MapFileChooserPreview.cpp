#include "MapFileChooserPreview.h"

#include "imap.h"
#include "i18n.h"
#include "itextstream.h"

#include "registry/registry.h"
#include "brush/BrushModule.h"
#include "gtkutil/ScrolledFrame.h"
#include "selection/algorithm/Primitives.h"
#include "map/algorithm/WorldspawnArgFinder.h"

#include <gtkmm/textview.h>

namespace map {

MapFileChooserPreview::MapFileChooserPreview() :
	Gtk::VBox(false, 0),
	_preview(new ui::MapPreview)
{
	_preview->setSize(400, 400);

	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 0));

	vbox->pack_start(*_preview, false, false, 0);
	vbox->pack_start(createUsagePanel(), true, true, 0);

	pack_start(*vbox, true, true, 0);

	show_all();
}

MapFileChooserPreview::~MapFileChooserPreview()
{
	delete _preview;
}

Gtk::Widget& MapFileChooserPreview::getPreviewWidget()
{
	return *this;
}

// Create the entity usage information panel
Gtk::Widget& MapFileChooserPreview::createUsagePanel()
{
	// Create a GtkTextView
	_usageInfo = Gtk::manage(new Gtk::TextView);
	_usageInfo->set_wrap_mode(Gtk::WRAP_WORD);
	_usageInfo->set_editable(false);

	return *Gtk::manage(new gtkutil::ScrolledFrame(*_usageInfo));
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

	_preview->initialisePreview();
	_preview->queue_draw();

	// Always have the preview active
	fileChooser.setPreviewActive(true);

	updateUsageInfo();
}

void MapFileChooserPreview::updateUsageInfo()
{
	// Get the underlying buffer object
	Glib::RefPtr<Gtk::TextBuffer> buf = _usageInfo->get_buffer();

	std::string usage("");

	if (_mapResource != NULL && _mapResource->getNode() != NULL)
	{
		// Retrieve the root node
		scene::INodePtr root = _mapResource->getNode();

		// Traverse the root to find the worldspawn
		WorldspawnArgFinder finder("editor_description");
		root->traverse(finder);

		usage = finder.getFoundValue();

		if (usage.empty())
		{
			usage = _("<no description>");
		}
	}

	buf->set_text(usage);
}

bool MapFileChooserPreview::setMapName(const std::string& name)
{
	_mapName = name;
	_mapResource = GlobalMapResourceManager().capture(_mapName);

	bool success = false;

	if (_mapResource == NULL)
	{
		// NULLify the preview map root on failure
		_preview->setRootNode(scene::INodePtr());
		return success;
	}

	// Suppress the map loading dialog to avoid user
	// getting stuck in the "drag filename" operation
    {
        registry::ScopedKeyChanger<bool> changer(
            RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG, true
        );

        if (_mapResource->load())
        {
            // Get the node from the resource
            scene::INodePtr root = _mapResource->getNode();

            assert(root != NULL);

            // Set the new rootnode
            _preview->setRootNode(root);

            success = true;
        }
        else
        {
            // Map load failed
            rWarning() << "Could not load map: " << _mapName << std::endl;
        }
    }

	return success;
}

} // namespace map
