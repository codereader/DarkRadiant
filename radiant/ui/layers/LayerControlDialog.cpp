#include "LayerControlDialog.h"

#include "ieventmanager.h"
#include "iregistry.h"
#include "stream/textstream.h"
#include "layers/LayerSystem.h"

#include <gtk/gtk.h>

namespace ui {

	namespace {
		const std::string RKEY_ROOT = "user/ui/layers/controlDialog/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	}

LayerControlDialog::LayerControlDialog() :
	PersistentTransientWindow("Layers", GlobalRadiant().getMainWindow(), true),
	_controlVBox(gtk_vbox_new(FALSE, 3))
{
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

	// Add two dummy layers
	scene::getLayerSystem().createLayer("TestLayer1");
	scene::getLayerSystem().createLayer("TestLayer2");
}

void LayerControlDialog::populateWindow() {
	// Create the "master" vbox 
	GtkWidget* overallVBox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(getWindow()), overallVBox);

	// Add the LayerControl vbox to the window
	gtk_box_pack_start(GTK_BOX(overallVBox), _controlVBox, FALSE, FALSE, 0);

	// Add the option buttons ("Create Laye", etc.) to the window
	gtk_box_pack_start(GTK_BOX(overallVBox), createButtons(), FALSE, FALSE, 0);
}

GtkWidget* LayerControlDialog::createButtons() {
	GtkWidget* buttonVBox = gtk_vbox_new(FALSE, 0);

	GtkWidget* createButton = gtk_button_new_from_stock(GTK_STOCK_NEW);
	gtk_widget_set_size_request(createButton, 100, -1);
	gtk_box_pack_start(GTK_BOX(buttonVBox), createButton, FALSE, FALSE, 0);

	GtkWidget* deleteButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_size_request(deleteButton, 100, -1);
	gtk_box_pack_start(GTK_BOX(buttonVBox), deleteButton, FALSE, FALSE, 0);

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
		gtk_container_remove(GTK_CONTAINER(_controlVBox), (*i)->getWidget());
	}

	// Remove all previously allocated layercontrols
	_layerControls.clear();
	
	// Local helper class for populating the window
	class LayerControlPopulator :
		public scene::LayerSystem::Visitor
	{
		LayerControls& _layerControls;
		GtkWidget* _vbox;
	public:
		LayerControlPopulator(LayerControls& layerControls, GtkWidget* vbox) :
			_layerControls(layerControls),
			_vbox(vbox)
		{}

		void visit(int layerID, std::string layerName) {
			// Create a new layercontrol for each visited layer
			LayerControlPtr control(new LayerControl(layerID));

			// Store the object locally
			_layerControls.push_back(control);

			gtk_box_pack_start(
				GTK_BOX(_vbox),
				control->getWidget(),
				FALSE, FALSE, 0
			);
		}
	} populator(_layerControls, _controlVBox);

	scene::getLayerSystem().foreachLayer(populator);
}

void LayerControlDialog::update() {
	// Broadcast the update() call
	for (LayerControls::iterator i = _layerControls.begin();
		 i != _layerControls.end(); i++)
	{
		(*i)->update();
	}
}

void LayerControlDialog::toggle() {
	Instance().toggleDialog();
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

} // namespace ui
