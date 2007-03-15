#include "SREditor.h"

#include "iregistry.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "qerplugin.h"
#include "ieventmanager.h"
#include "selectionlib.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include <gtk/gtk.h>

#include <iostream>

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Stim/Response Editor";
		
		const std::string RKEY_ROOT = "user/ui/stimResponseEditor/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		
		const std::string LABEL_STIMRESPONSE_LIST = "Stims/Responses";
	}

StimResponseEditor::StimResponseEditor() :
	_entity(NULL)
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow(), false);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	// Create the widgets
	populateWindow();
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(_dialog));
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
	_windowPosition.applyPosition();
}

void StimResponseEditor::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	gtk_widget_hide(_dialog);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(_dialog));
}

void StimResponseEditor::toggleWindow() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_dialog)) {
		// Save the window position, to make sure
		_windowPosition.readPosition();
		gtk_widget_hide_all(_dialog);
	}
	else {
		// Restore the position
		_windowPosition.applyPosition();
		// Scan the selection for entities
		rescanSelection();
		// Now show the dialog window again
		gtk_widget_show_all(_dialog);
	}
}

void StimResponseEditor::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(false, 6);
	gtk_container_add(GTK_CONTAINER(_dialog), _dialogVBox);
	
	// Create the title label (bold font)
	GtkWidget* topLabel = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_STIMRESPONSE_LIST + "</span>"
    );
    gtk_box_pack_start(GTK_BOX(_dialogVBox), topLabel, true, true, 0);
	
	GtkWidget* srHBox = gtk_hbox_new(false, 6);
	
	// Pack it into an alignment so that it is indented
	GtkWidget* srAlignment = gtkutil::LeftAlignment(GTK_WIDGET(srHBox), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(_dialogVBox), GTK_WIDGET(srAlignment), true, true, 0);
	
	_entitySRView = gtk_tree_view_new();
	gtk_box_pack_start(GTK_BOX(srHBox), _entitySRView, false, false, 0);
	
	// Cast the helper class onto a ListStore and create a new treeview
	GtkListStore* stimListStore = _stimTypes;
	_stimTypeList = gtk_combo_box_new_with_model(GTK_TREE_MODEL(stimListStore));
	g_object_unref(stimListStore); // tree view owns the reference now
	
	// Add the cellrenderer for the name
	GtkCellRenderer* nameRenderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_stimTypeList), nameRenderer, true);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_stimTypeList), nameRenderer, "text", 1);
	
	gtk_box_pack_start(GTK_BOX(_dialogVBox), _stimTypeList, false, false, 0);
}

void StimResponseEditor::update() {
	gtk_widget_set_sensitive(_dialogVBox, _entity != NULL);
}

void StimResponseEditor::rescanSelection() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	_entity = NULL;
	
	if (info.entityCount == 1 && info.totalCount == 1) {
		// Get the entity instance
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		scene::Node& node = instance.path().top();
		_entity = Node_getEntity(node);
		if (_entity != NULL) {
			globalOutputStream() << "Entity: " << _entity->getKeyValue("classname").c_str() << "\n";
		}
	}
	
	// Update the widgets
	update();
}

void StimResponseEditor::selectionChanged(scene::Instance& instance) {
	rescanSelection();
}

// Static GTK Callbacks
gboolean StimResponseEditor::onDelete(GtkWidget* widget, GdkEvent* event, StimResponseEditor* self) {
	// Toggle the visibility of the window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}

// Static command target
void StimResponseEditor::toggle() {
	Instance().toggleWindow();
}

StimResponseEditor& StimResponseEditor::Instance() {
	static StimResponseEditor _instance;
	return _instance;
}

} // namespace ui
