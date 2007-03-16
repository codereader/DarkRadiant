#include "SREditor.h"

#include "iregistry.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "qerplugin.h"
#include "ieventmanager.h"
#include "selectionlib.h"
#include "gtkutil/image.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/ScrolledFrame.h"
#include <gtk/gtk.h>

#include <iostream>

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Stim/Response Editor";
		
		const std::string RKEY_ROOT = "user/ui/stimResponseEditor/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		
		const std::string LABEL_STIMRESPONSE_LIST = "Stims/Responses";
		const std::string LABEL_ADD_STIMRESPONSE = "Add Stim/Response";
		const std::string ICON_STIM = "sr_stim.png";
		const std::string ICON_RESPONSE = "sr_response.png";
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
	gtk_widget_set_size_request(_entitySRView, 280, 240);
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_entitySRView),
		gtkutil::TextColumn("#", 0)
	);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_entitySRView),
		gtkutil::TextColumn("S/R", 1)
	);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_entitySRView),
		gtkutil::IconTextColumn("Type", 2, 3)
	);
	
	g_signal_connect(G_OBJECT(_entitySRView), "cursor-changed", G_CALLBACK(onSelectionChange), this);
	
	gtk_box_pack_start(GTK_BOX(srHBox), 
		gtkutil::ScrolledFrame(_entitySRView), true, true, 0);
	
	gtk_box_pack_start(GTK_BOX(srHBox), createSRWidgets(), true, true, 0);
	
	// Create the title label (bold font)
	GtkWidget* addLabel = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_ADD_STIMRESPONSE + "</span>"
    );
    gtk_box_pack_start(GTK_BOX(_dialogVBox), addLabel, true, true, 0);
	
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

GtkWidget* StimResponseEditor::createSRWidgets() {
	_srWidgets.vbox = gtk_vbox_new(false, 6);
	
	// Create the buttons
	_srWidgets.stimButton = gtk_toggle_button_new();
	_srWidgets.respButton = gtk_toggle_button_new();
	g_signal_connect(G_OBJECT(_srWidgets.stimButton), "toggled", G_CALLBACK(onTypeChange), this);
	g_signal_connect(G_OBJECT(_srWidgets.respButton), "toggled", G_CALLBACK(onTypeChange), this);
	
	GtkWidget* stimImg = gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(ICON_STIM));
	GtkWidget* respImg = gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(ICON_RESPONSE));
	gtk_widget_set_size_request(stimImg, 20, -1);
	gtk_widget_set_size_request(respImg, 20, -1);
	
	GtkWidget* stimLabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(stimLabel), "<b>Stim</b>");
	
	GtkWidget* respLabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(respLabel), "<b>Response</b>");
	
	GtkWidget* stimBtnHbox = gtk_hbox_new(false, 3);
	gtk_box_pack_start(GTK_BOX(stimBtnHbox), stimImg, false, false, 0);
	gtk_box_pack_start(GTK_BOX(stimBtnHbox), stimLabel, false, false, 0);
	gtk_container_add(GTK_CONTAINER(_srWidgets.stimButton), stimBtnHbox);
	
	GtkWidget* respBtnHbox = gtk_hbox_new(false, 3);
	gtk_box_pack_start(GTK_BOX(respBtnHbox), respImg, false, false, 0);
	gtk_box_pack_start(GTK_BOX(respBtnHbox), respLabel, false, false, 0);
	gtk_container_add(GTK_CONTAINER(_srWidgets.respButton), respBtnHbox);
	
	// combine the buttons to a hbox
	GtkWidget* btnHbox = gtk_hbox_new(true, 6);
	gtk_box_pack_start(GTK_BOX(btnHbox), _srWidgets.stimButton, true, true, 0);
	gtk_box_pack_start(GTK_BOX(btnHbox), _srWidgets.respButton, true, true, 0);
	
	// Pack the button Hbox to the SRWidgets
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), btnHbox, false, false, 0);
	
	// Active
	GtkWidget* activeHBox = gtk_hbox_new(false, 0);
	GtkWidget* activeLabel = gtkutil::LeftAlignedLabel("Active");
	_srWidgets.active = gtk_check_button_new();
	gtk_box_pack_start(GTK_BOX(activeHBox), _srWidgets.active, false, false, 0);
	gtk_box_pack_start(GTK_BOX(activeHBox), activeLabel, false, false, 0);
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), activeHBox, false, false, 0);
	
	// Use Bounds
	GtkWidget* boundsHBox = gtk_hbox_new(false, 0);
	GtkWidget* boundsLabel = gtkutil::LeftAlignedLabel("Use bounds");
	_srWidgets.useBounds = gtk_check_button_new();
	gtk_box_pack_start(GTK_BOX(boundsHBox), _srWidgets.useBounds, false, false, 0);
	gtk_box_pack_start(GTK_BOX(boundsHBox), boundsLabel, false, false, 0);
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), boundsHBox, false, false, 0);
	
	// Radius
	GtkWidget* radiusHBox = gtk_hbox_new(false, 0);
	_srWidgets.radiusToggle = gtk_check_button_new();
	GtkWidget* radiusLabel = gtkutil::LeftAlignedLabel("Radius");
	gtk_widget_set_size_request(radiusLabel, 90, -1);
	_srWidgets.radiusEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(radiusHBox), _srWidgets.radiusToggle, false, false, 0);
	gtk_box_pack_start(GTK_BOX(radiusHBox), radiusLabel, false, false, 0);
	gtk_box_pack_start(GTK_BOX(radiusHBox), _srWidgets.radiusEntry, true, true, 0);
	
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), radiusHBox, false, false, 0);
	
	// Time Interval
	GtkWidget* timeHBox = gtk_hbox_new(false, 0);
	_srWidgets.timeIntToggle = gtk_check_button_new();
	GtkWidget* timeLabel = gtkutil::LeftAlignedLabel("Time interval");
	gtk_widget_set_size_request(timeLabel, 90, -1);
	_srWidgets.timeIntEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(timeHBox), _srWidgets.timeIntToggle, false, false, 0);
	gtk_box_pack_start(GTK_BOX(timeHBox), timeLabel, false, false, 0);
	gtk_box_pack_start(GTK_BOX(timeHBox), _srWidgets.timeIntEntry, true, true, 0);
	
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), timeHBox, false, false, 0);
	
	// Model
	GtkWidget* modelHBox = gtk_hbox_new(false, 0);
	_srWidgets.modelToggle = gtk_check_button_new();
	GtkWidget* modelLabel = gtkutil::LeftAlignedLabel("Model");
	gtk_widget_set_size_request(modelLabel, 90, -1);
	_srWidgets.modelEntry = gtk_entry_new();
	
	gtk_box_pack_start(GTK_BOX(modelHBox), _srWidgets.modelToggle, false, false, 0);
	gtk_box_pack_start(GTK_BOX(modelHBox), modelLabel, false, false, 0);
	gtk_box_pack_start(GTK_BOX(modelHBox), _srWidgets.modelEntry, true, true, 0);
	
	gtk_box_pack_start(GTK_BOX(_srWidgets.vbox), modelHBox, false, false, 0);
	
	return _srWidgets.vbox;
}

void StimResponseEditor::update() {
	gtk_widget_set_sensitive(_dialogVBox, _entity != NULL);
	
	updateSRWidgets();
}

void StimResponseEditor::rescanSelection() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	_entity = NULL;
	_srEntity = SREntityPtr();
	
	if (info.entityCount == 1 && info.totalCount == 1) {
		// Get the entity instance
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		scene::Node& node = instance.path().top();
		
		_entity = Node_getEntity(node);
		
		_srEntity = SREntityPtr(new SREntity(_entity));
		
		// Cast the SREntity onto a liststore and pack it into the treeview
		GtkListStore* listStore = *_srEntity;
		gtk_tree_view_set_model(GTK_TREE_VIEW(_entitySRView), GTK_TREE_MODEL(listStore));
		g_object_unref(listStore);
	}
	
	// Update the widgets
	update();
}

void StimResponseEditor::selectionChanged(scene::Instance& instance) {
	rescanSelection();
}

void StimResponseEditor::updateSRWidgets() {
	
}

// Static GTK Callbacks
gboolean StimResponseEditor::onDelete(GtkWidget* widget, GdkEvent* event, StimResponseEditor* self) {
	// Toggle the visibility of the window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}

void StimResponseEditor::onSelectionChange(GtkTreeView* treeView, StimResponseEditor* self) {
	self->updateSRWidgets();
}

void StimResponseEditor::onTypeChange(GtkToggleButton* toggleButton, StimResponseEditor* self) {
	
	// Invert the other button
	if (GTK_WIDGET(toggleButton) == self->_srWidgets.stimButton) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(self->_srWidgets.respButton),
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->_srWidgets.stimButton))
		);
	}
	else {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(self->_srWidgets.stimButton),
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->_srWidgets.respButton))
		);
	}
	
	self->updateSRWidgets();
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
