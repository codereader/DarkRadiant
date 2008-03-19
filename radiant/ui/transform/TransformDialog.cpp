#include "TransformDialog.h"

#include "iregistry.h"
#include "ieventmanager.h"
#include "selectionlib.h"

#include <gtk/gtk.h>

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ControlButton.h"

#include "mainframe.h"
#include "selection/algorithm/Transformation.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Arbitrary Transformation";
		const std::string LABEL_ROTATION = "Rotation";
		const std::string LABEL_SCALE = "Scale";
		
		const std::string LABEL_ROTX = "X-Axis Rotate:";
		const std::string LABEL_ROTY = "Y-Axis Rotate:";
		const std::string LABEL_ROTZ = "Z-Axis Rotate:";
		
		const std::string LABEL_SCALEX = "X-Axis Scale:";
		const std::string LABEL_SCALEY = "Y-Axis Scale:";
		const std::string LABEL_SCALEZ = "Z-Axis Scale:";
		
		const char* LABEL_STEP = "Step:";
		
		const std::string RKEY_ROOT = "user/ui/transformDialog/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		const std::string RKEY_ROTX_STEP = RKEY_ROOT + "rotXStep";
		const std::string RKEY_ROTY_STEP = RKEY_ROOT + "rotYStep";
		const std::string RKEY_ROTZ_STEP = RKEY_ROOT + "rotZStep";
		
		const std::string RKEY_SCALEX_STEP = RKEY_ROOT + "scaleXStep";
		const std::string RKEY_SCALEY_STEP = RKEY_ROOT + "scaleYStep";
		const std::string RKEY_SCALEZ_STEP = RKEY_ROOT + "scaleZStep";
	}

TransformDialog::TransformDialog() 
: gtkutil::PersistentTransientWindow(WINDOW_TITLE, MainFrame_getWindow(), true),
  _selectionInfo(GlobalSelectionSystem().getSelectionInfo())
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	
	// Update the widget sensitivity
	update();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
}

void TransformDialog::onRadiantShutdown() {
	
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));

	// Destroy the dialog
	destroy();
}

TransformDialogPtr& TransformDialog::InstancePtr() {
	static TransformDialogPtr _instancePtr;
	
	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = TransformDialogPtr(new TransformDialog);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}
	
	return _instancePtr;
}

TransformDialog& TransformDialog::Instance() {
	return *InstancePtr();
}

// The command target
void TransformDialog::toggle() {
	Instance().toggleDialog();
}

void TransformDialog::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(false, 6);
	gtk_container_add(GTK_CONTAINER(getWindow()), _dialogVBox);
	
	// Create the rotation label (bold font)
	_rotateLabel = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_ROTATION + "</span>"
    );
    gtk_box_pack_start(GTK_BOX(_dialogVBox), _rotateLabel, false, false, 0);
    
    // Setup the table with default spacings
	_rotateTable = GTK_TABLE(gtk_table_new(3, 2, false));
    gtk_table_set_col_spacings(_rotateTable, 12);
    gtk_table_set_row_spacings(_rotateTable, 6);
    
    // Pack it into an alignment so that it is indented
	GtkWidget* rotAlignment = gtkutil::LeftAlignment(GTK_WIDGET(_rotateTable), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(_dialogVBox), GTK_WIDGET(rotAlignment), false, false, 0);
    
    _entries["rotateX"] = createEntryRow(LABEL_ROTX, _rotateTable, 0, true, 0);
    _entries["rotateY"] = createEntryRow(LABEL_ROTY, _rotateTable, 1, true, 1);
    _entries["rotateZ"] = createEntryRow(LABEL_ROTZ, _rotateTable, 2, true, 2);
    
    // Create the rotation label (bold font)
	_scaleLabel = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_SCALE + "</span>"
    );
    gtk_box_pack_start(GTK_BOX(_dialogVBox), _scaleLabel, false, false, 0);
    
    // Setup the table with default spacings
	_scaleTable = GTK_TABLE(gtk_table_new(3, 2, false));
    gtk_table_set_col_spacings(_scaleTable, 12);
    gtk_table_set_row_spacings(_scaleTable, 6);
    
    // Pack it into an alignment so that it is indented
	GtkWidget* scaleAlignment = gtkutil::LeftAlignment(GTK_WIDGET(_scaleTable), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(_dialogVBox), GTK_WIDGET(scaleAlignment), false, false, 0);
    
    _entries["scaleX"] = createEntryRow(LABEL_SCALEX, _scaleTable, 0, false, 0);
    _entries["scaleY"] = createEntryRow(LABEL_SCALEY, _scaleTable, 1, false, 1);
    _entries["scaleZ"] = createEntryRow(LABEL_SCALEZ, _scaleTable, 2, false, 2);
    
    // Connect the step values to the according registry values
	_connector.connectGtkObject(GTK_OBJECT(_entries["rotateX"].step), RKEY_ROTX_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_entries["rotateY"].step), RKEY_ROTY_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_entries["rotateZ"].step), RKEY_ROTZ_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_entries["scaleX"].step), RKEY_SCALEX_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_entries["scaleY"].step), RKEY_SCALEY_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_entries["scaleZ"].step), RKEY_SCALEZ_STEP);
    
    // Connect all the arrow buttons
    for (EntryRowMap::iterator i = _entries.begin(); i != _entries.end(); i++) {
    	EntryRow& row = i->second;
    	
    	// Cast the ControlButtons onto GtkWidgets
    	GtkWidget* smallerButton = *row.smaller; 
    	GtkWidget* largerButton = *row.larger;
    	
    	// Pass a CoordRow pointer to the callback, that's all it will need to update
    	g_signal_connect(G_OBJECT(smallerButton), "clicked", G_CALLBACK(onClickSmaller), &row);
    	g_signal_connect(G_OBJECT(largerButton), "clicked", G_CALLBACK(onClickLarger), &row);
    }
}

TransformDialog::EntryRow TransformDialog::createEntryRow(
	const std::string& label, GtkTable* table, int row, bool isRotator, int axis) 
{
	EntryRow entryRow;
	
	entryRow.isRotator = isRotator;
	entryRow.axis = axis;
	
	// greebo: The rotation direction is reversed for X and Z rotations
	// This has no mathematical meaning, it's just for looking right.
	entryRow.direction = (isRotator && axis != 1) ? -1 : 1;
	
	// Create the label
	entryRow.label = gtkutil::LeftAlignedLabel(label);
	gtk_table_attach_defaults(table, entryRow.label, 0, 1, row, row+1);
	
	entryRow.hbox = gtk_hbox_new(false, 6);
	
	// Create the control buttons (zero spacing hbox)
	{
		GtkWidget* hbox = gtk_hbox_new(true, 0);
		
		entryRow.smaller = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalRadiant().getLocalPixbuf("arrow_left.png"))
		);
		gtk_widget_set_size_request(*entryRow.smaller, 15, 24);
		gtk_box_pack_start(GTK_BOX(hbox), *entryRow.smaller, false, false, 0);
		
		entryRow.larger = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalRadiant().getLocalPixbuf("arrow_right.png"))
		);
		gtk_widget_set_size_request(*entryRow.larger, 15, 24);
		gtk_box_pack_start(GTK_BOX(hbox), *entryRow.larger, false, false, 0); 
		
		gtk_box_pack_start(GTK_BOX(entryRow.hbox), hbox, false, false, 0);
	}
	
	// Create the label
	entryRow.stepLabel = gtkutil::LeftAlignedLabel(LABEL_STEP); 
	gtk_box_pack_start(GTK_BOX(entryRow.hbox), entryRow.stepLabel, false, false, 0);
	
	// Create the entry field
	entryRow.step = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entryRow.step), 5);
	g_signal_connect(G_OBJECT(entryRow.step), "changed", G_CALLBACK(onStepChanged), this);
	
	gtk_box_pack_start(GTK_BOX(entryRow.hbox), entryRow.step, false, false, 0);
		
	// Pack the hbox into the table
	gtk_table_attach_defaults(table, entryRow.hbox, 1, 2, row, row+1);
	
	// Return the filled structure
	return entryRow;
}

void TransformDialog::toggleDialog() {
	if (isVisible())
		hide();
	else
		show();
}

// Pre-hide callback
void TransformDialog::_preHide() {
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void TransformDialog::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();
	// Update the widget values
	update();
}

void TransformDialog::update() {
	// Check if there is anything selected
	bool rotSensitive = (_selectionInfo.totalCount > 0);
	bool scaleSensitive = (_selectionInfo.totalCount > 0 && _selectionInfo.entityCount == 0);

	gtk_widget_set_sensitive(GTK_WIDGET(_dialogVBox), rotSensitive || scaleSensitive);

	// set the sensitivity of the scale/rotation widgets
	gtk_widget_set_sensitive(GTK_WIDGET(_rotateLabel), rotSensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(_rotateTable), rotSensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(_scaleLabel), scaleSensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(_scaleTable), scaleSensitive);
}

void TransformDialog::selectionChanged(const scene::INodePtr& node, bool isComponent) {
	update();
}

void TransformDialog::saveToRegistry() {
	// Pass the call to the RegistryConnector
	_connector.exportValues();
}

void TransformDialog::onStepChanged(GtkEditable* editable, TransformDialog* self) {
	// Tell the class instance to save its contents into the registry
	self->saveToRegistry();
} 

void TransformDialog::onClickLarger(GtkWidget* button, EntryRow* row) {
	// Get the current step increment
	float step = strToFloat(gtk_entry_get_text(GTK_ENTRY(row->step)));
	
	// Determine the action
	if (row->isRotator) {
		// Do a rotation
		Vector3 eulerXYZ;
		
		// Store the value into the right axis
		eulerXYZ[row->axis] = step * row->direction;
		
		// Pass the call to the algorithm functions
		selection::algorithm::rotateSelected(eulerXYZ);
	}
	else {
		// Do a scale
		Vector3 scaleXYZ(1,1,1);
		
		// Store the value into the right axis
		scaleXYZ[row->axis] = step;
		
		// Pass the call to the algorithm functions
		selection::algorithm::scaleSelected(scaleXYZ);
	}
}

void TransformDialog::onClickSmaller(GtkWidget* button, EntryRow* row) {
	// Get the current value and the step increment
	float step = strToFloat(gtk_entry_get_text(GTK_ENTRY(row->step)));
	
	// Determine the action
	if (row->isRotator) {
		// Do a rotation
		Vector3 eulerXYZ;
		
		// Store the value into the right axis
		eulerXYZ[row->axis] = -step * row->direction;
		
		// Pass the call to the algorithm functions
		selection::algorithm::rotateSelected(eulerXYZ);
	}
	else {
		// Do a scale
		Vector3 scaleXYZ(1,1,1);
		
		// Store the value into the right axis
		scaleXYZ[row->axis] = 1/step;
		
		// Pass the call to the algorithm functions
		selection::algorithm::scaleSelected(scaleXYZ);
	}
}

} // namespace ui
