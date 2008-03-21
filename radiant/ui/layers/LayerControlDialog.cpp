#include "LayerControlDialog.h"

#include "ieventmanager.h"
#include "iregistry.h"
#include "stream/textstream.h"

namespace ui {

	namespace {
		const std::string RKEY_ROOT = "user/ui/layers/controlDialog/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	}

LayerControlDialog::LayerControlDialog() :
	PersistentTransientWindow("Layers", GlobalRadiant().getMainWindow(), true)
{
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));

	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
}

void LayerControlDialog::toggleDialog() {
	if (isVisible()) {
		hide();
	}
	else {
		show();
	}
}

void LayerControlDialog::update() {

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

} // namespace ui
