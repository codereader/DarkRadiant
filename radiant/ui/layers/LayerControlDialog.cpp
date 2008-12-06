#include "LayerControlDialog.h"

#include <gtk/gtk.h>

#include "ieventmanager.h"
#include "ilayer.h"
#include "iregistry.h"
#include "stream/textstream.h"

#include "gtkutil/dialog.h"
#include "gtkutil/EntryAbortedException.h"

#include "layers/LayerSystem.h"

namespace ui {

	namespace {
		const std::string RKEY_ROOT = "user/ui/layers/controlDialog/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	}

LayerControlDialog::LayerControlDialog() :
	PersistentTransientWindow("Layers", GlobalRadiant().getMainWindow(), true),
	_controlContainer(gtk_table_new(1, 3, FALSE))
{
	gtk_table_set_row_spacings(GTK_TABLE(_controlContainer), 3);
	gtk_table_set_col_spacings(GTK_TABLE(_controlContainer), 3);

	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(
		GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG
	);

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));

	populateWindow();

	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
}

void LayerControlDialog::populateWindow() {
	// Create the "master" vbox 
	GtkWidget* overallVBox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(getWindow()), overallVBox);

	// Add the LayerControl vbox to the window
	gtk_box_pack_start(GTK_BOX(overallVBox), _controlContainer, FALSE, FALSE, 0);

	// Add the option buttons ("Create Layer", etc.) to the window
	gtk_box_pack_start(GTK_BOX(overallVBox), createButtons(), FALSE, FALSE, 0);
}

GtkWidget* LayerControlDialog::createButtons() {
	GtkWidget* buttonVBox = gtk_vbox_new(FALSE, 6);

	// Show all / hide all buttons
	GtkWidget* hideShowBox = gtk_hbox_new(TRUE, 6);

	_showAllLayers = gtk_button_new_with_label("Show all");
	_hideAllLayers = gtk_button_new_with_label("Hide all");

	g_signal_connect(G_OBJECT(_showAllLayers), "clicked", G_CALLBACK(onShowAllLayers), this);
	g_signal_connect(G_OBJECT(_hideAllLayers), "clicked", G_CALLBACK(onHideAllLayers), this);

	gtk_box_pack_start(GTK_BOX(hideShowBox), _showAllLayers, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hideShowBox), _hideAllLayers, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(buttonVBox), hideShowBox, FALSE, FALSE, 0);

	// Create layer button
	GtkWidget* createButton = gtk_button_new_from_stock(GTK_STOCK_NEW);
	g_signal_connect(G_OBJECT(createButton), "clicked", G_CALLBACK(onCreateLayer), this);
	gtk_widget_set_size_request(createButton, 100, -1);
	gtk_box_pack_start(GTK_BOX(buttonVBox), createButton, FALSE, FALSE, 0);

	return buttonVBox;
}

void LayerControlDialog::toggleDialog() {
	if (isVisible()) {
		hide();
	}
	else {
		show();
	}
}

void LayerControlDialog::refresh() {
	// Remove the widgets from the vbox first
	for (LayerControls::iterator i = _layerControls.begin(); 
		 i != _layerControls.end(); i++)
	{
		gtk_container_remove(GTK_CONTAINER(_controlContainer), (*i)->getToggle());
		gtk_container_remove(GTK_CONTAINER(_controlContainer), (*i)->getLabelButton());
		gtk_container_remove(GTK_CONTAINER(_controlContainer), (*i)->getButtons());
	}

	// Remove all previously allocated layercontrols
	_layerControls.clear();
	
	// Local helper class for populating the window
	class LayerControlPopulator :
		public scene::LayerSystem::Visitor
	{
		LayerControls& _layerControls;
	public:
		LayerControlPopulator(LayerControls& layerControls) :
			_layerControls(layerControls)
		{}

		void visit(int layerID, std::string layerName) {
			// Create a new layercontrol for each visited layer
			LayerControlPtr control(new LayerControl(layerID));

			// Store the object locally
			_layerControls.push_back(control);
		}
	} populator(_layerControls);

	// Traverse the layers
	scene::getLayerSystem().foreachLayer(populator);

	gtk_table_resize(GTK_TABLE(_controlContainer), static_cast<guint>(_layerControls.size()), 3);

	int c = 0;
	for (LayerControls::iterator i = _layerControls.begin(); 
		 i != _layerControls.end(); i++, c++)
	{
		gtk_table_attach(GTK_TABLE(_controlContainer), (*i)->getToggle(), 
			0, 1, c, c+1, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);

		gtk_table_attach_defaults(GTK_TABLE(_controlContainer), (*i)->getLabelButton(), 1, 2, c, c+1);

		gtk_table_attach(GTK_TABLE(_controlContainer), (*i)->getButtons(), 
			2, 3, c, c+1, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
	}

	// Make sure the newly added items are visible
	gtk_widget_show_all(_controlContainer);

	update();
}

void LayerControlDialog::update() {
	// Broadcast the update() call
	for (LayerControls::iterator i = _layerControls.begin();
		 i != _layerControls.end(); i++)
	{
		(*i)->update();
	}

	// Update the show/hide all button sensitiveness

	class CheckAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		std::size_t numVisible;
		std::size_t numHidden;

		CheckAllLayersWalker() :
			numVisible(0),
			numHidden(0)
		{}

		void visit(int layerID, std::string layerName) {
			if (GlobalLayerSystem().layerIsVisible(layerID)) {
				numVisible++;
			}
			else {
				numHidden++;
			}
		}
	};

	CheckAllLayersWalker visitor;
	GlobalLayerSystem().foreachLayer(visitor);

	gtk_widget_set_sensitive(_showAllLayers, visitor.numHidden > 0);
	gtk_widget_set_sensitive(_hideAllLayers, visitor.numVisible > 0);
}

void LayerControlDialog::toggle() {
	Instance().toggleDialog();
}

void LayerControlDialog::init() {
	// Lookup the stored window information in the registry
	xml::NodeList list = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);

	if (!list.empty()) {
		xml::Node& node = list[0];

		if (node.getAttributeValue("visible") == "1") {
			// Show dialog
			Instance().show();
		}
	}
}

void LayerControlDialog::onRadiantShutdown() {
	globalOutputStream() << "LayerControlDialog shutting down.\n";

	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);

	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));

	// Write the visibility status to the registry
	if (isVisible()) {
		node.setAttributeValue("visible", "1");
	}

	// Destroy the window (after it has been disconnected from the Eventmanager)
	destroy();
}

LayerControlDialogPtr& LayerControlDialog::InstancePtr() {
	static LayerControlDialogPtr _instancePtr;

	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = LayerControlDialogPtr(new LayerControlDialog);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}

	return _instancePtr;
}

LayerControlDialog& LayerControlDialog::Instance() {
	return *InstancePtr();
}

// TransientWindow callbacks
void LayerControlDialog::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();
	// Re-populate the dialog
	refresh();
}

void LayerControlDialog::_preHide() {
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Static GTK callbacks
void LayerControlDialog::onCreateLayer(GtkWidget* button, LayerControlDialog* self) {
	while (true) {
		// Query the name of the new layer from the user
		std::string layerName;

		try {
			layerName = gtkutil::textEntryDialog(
				"Enter Name", 
				"Enter Layer Name", 
				GTK_WINDOW(self->getWindow())
			);
		}
		catch (gtkutil::EntryAbortedException e) {
			break;
		}

		// Attempt to create the layer, this will return -1 if the operation fails
		int layerID = scene::getLayerSystem().createLayer(layerName);

		if (layerID != -1) {
			// Reload the widgets, we're done here
			self->refresh();
			break;
		}
		else {
			// Wrong name, let the user try again
			gtkutil::errorDialog("This name already exists.", GTK_WINDOW(self->getWindow()));
			continue; 
		}
	}
}

void LayerControlDialog::onShowAllLayers(GtkWidget* button, LayerControlDialog* self) {
	// Local helper class
	class ShowAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		void visit(int layerID, std::string layerName) {
			GlobalLayerSystem().setLayerVisibility(layerID, true);
		}
	};

	ShowAllLayersWalker walker;
	GlobalLayerSystem().foreachLayer(walker);
}

void LayerControlDialog::onHideAllLayers(GtkWidget* button, LayerControlDialog* self) {
	// Local helper class
	class HideAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		void visit(int layerID, std::string layerName) {
			GlobalLayerSystem().setLayerVisibility(layerID, false);
		}
	};

	HideAllLayersWalker walker;
	GlobalLayerSystem().foreachLayer(walker);
}

} // namespace ui
