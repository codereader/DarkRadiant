#include "PatchInspector.h"

#include "iregistry.h"
#include "ieventmanager.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "selectionlib.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ControlButton.h"

#include "mainframe.h"
#include "patch/PatchNode.h"
#include "selection/algorithm/Primitives.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Patch Inspector";
		const std::string LABEL_CONTROL_VERTICES = "Patch Control Vertices";
		const std::string LABEL_COORDS = "Coordinates";
		const std::string LABEL_ROW = "Row:";
		const std::string LABEL_COL = "Column:";
		const std::string LABEL_TESSELATION = "Patch Tesselation";
		const std::string LABEL_FIXED = "Fixed Subdivisions";
		const std::string LABEL_SUBDIVISION_X = "Horizontal:";
		const std::string LABEL_SUBDIVISION_Y = "Vertical:";
		const char* LABEL_STEP = "Step:";
		
		const float TESS_MIN = 1.0f;
		const float TESS_MAX = 32.0f;
		
		const std::string RKEY_ROOT = "user/ui/patch/patchInspector/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		const std::string RKEY_X_STEP = RKEY_ROOT + "xCoordStep";
		const std::string RKEY_Y_STEP = RKEY_ROOT + "yCoordStep";
		const std::string RKEY_Z_STEP = RKEY_ROOT + "zCoordStep";
		const std::string RKEY_S_STEP = RKEY_ROOT + "sCoordStep";
		const std::string RKEY_T_STEP = RKEY_ROOT + "tCoordStep";
	}

PatchInspector::PatchInspector() 
: gtkutil::PersistentTransientWindow(WINDOW_TITLE, MainFrame_getWindow(), true),
  _selectionInfo(GlobalSelectionSystem().getSelectionInfo()),
  _patchRows(0),
  _patchCols(0),
  _patch(NULL),
  _updateActive(false)
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
	
	// Update the widget status
	rescanSelection();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
}

PatchInspectorPtr& PatchInspector::InstancePtr() {
	static PatchInspectorPtr _instancePtr;
	
	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = PatchInspectorPtr(new PatchInspector);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}
	
	return _instancePtr;
}

void PatchInspector::onRadiantShutdown() {
	globalOutputStream() << "PatchInspector shutting down.\n";

	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Destroy the transient window
	destroy();
}

PatchInspector& PatchInspector::Instance() {
	return *InstancePtr();
}

void PatchInspector::populateWindow() {
	// Create the overall vbox
	GtkWidget* dialogVBox = gtk_vbox_new(false, 6);
	gtk_container_add(GTK_CONTAINER(getWindow()), dialogVBox);
	
	// Create the title label (bold font)
	_vertexChooser.title = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_CONTROL_VERTICES + "</span>"
    );
    gtk_box_pack_start(GTK_BOX(dialogVBox), _vertexChooser.title, false, false, 0);
    
    // Setup the table with default spacings
	_vertexChooser.table = GTK_TABLE(gtk_table_new(2, 2, false));
    gtk_table_set_col_spacings(_vertexChooser.table, 12);
    gtk_table_set_row_spacings(_vertexChooser.table, 6);
    
    // Pack it into an alignment so that it is indented
	GtkWidget* alignment = gtkutil::LeftAlignment(GTK_WIDGET(_vertexChooser.table), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(alignment), false, false, 0);
	
	// The vertex col and row chooser	
	_vertexChooser.rowLabel = gtkutil::LeftAlignedLabel(LABEL_ROW);
	gtk_table_attach_defaults(_vertexChooser.table, _vertexChooser.rowLabel, 0, 1, 0, 1);
		
	_vertexChooser.rowCombo = gtk_combo_box_new_text();
	gtk_widget_set_size_request(_vertexChooser.rowCombo, 100, -1);
	g_signal_connect(G_OBJECT(_vertexChooser.rowCombo), "changed", G_CALLBACK(onComboBoxChange), this);
	gtk_table_attach_defaults(_vertexChooser.table, _vertexChooser.rowCombo, 1, 2, 0, 1);
		
	_vertexChooser.colLabel = gtkutil::LeftAlignedLabel(LABEL_COL);
	gtk_table_attach_defaults(_vertexChooser.table, _vertexChooser.colLabel, 0, 1, 1, 2);
	
	_vertexChooser.colCombo = gtk_combo_box_new_text();
	gtk_widget_set_size_request(_vertexChooser.colCombo, 100, -1);
	g_signal_connect(G_OBJECT(_vertexChooser.colCombo), "changed", G_CALLBACK(onComboBoxChange), this);
	gtk_table_attach_defaults(_vertexChooser.table, _vertexChooser.colCombo, 1, 2, 1, 2);
	
	// Create the title label (bold font)
	_coordsLabel = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_COORDS + "</span>"
    );
    gtk_misc_set_padding(GTK_MISC(_coordsLabel), 0, 2);
    gtk_box_pack_start(GTK_BOX(dialogVBox), _coordsLabel, false, false, 0);
	
	// Setup the table with default spacings
	_coordsTable = GTK_TABLE(gtk_table_new(5, 2, false));
    gtk_table_set_col_spacings(_coordsTable, 12);
    gtk_table_set_row_spacings(_coordsTable, 6);
    
    // Pack it into an alignment so that it is indented
	GtkWidget* coordAlignment = gtkutil::LeftAlignment(GTK_WIDGET(_coordsTable), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(coordAlignment), false, false, 0);
    
    _coords["x"] = createCoordRow("X:", _coordsTable, 0);
    _coords["y"] = createCoordRow("Y:", _coordsTable, 1);
    _coords["z"] = createCoordRow("Z:", _coordsTable, 2);
    _coords["s"] = createCoordRow("S:", _coordsTable, 3);
    _coords["t"] = createCoordRow("T:", _coordsTable, 4);
    
    // Connect the step values to the according registry values
	_connector.connectGtkObject(GTK_OBJECT(_coords["x"].step), RKEY_X_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_coords["y"].step), RKEY_Y_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_coords["z"].step), RKEY_Z_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_coords["s"].step), RKEY_S_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_coords["t"].step), RKEY_T_STEP);
    
    // Connect all the arrow buttons
    for (CoordMap::iterator i = _coords.begin(); i != _coords.end(); i++) {
    	CoordRow& row = i->second;
    	
    	// Cast the ControlButtons onto GtkWidgets
    	GtkWidget* smallerButton = *row.smaller; 
    	GtkWidget* largerButton = *row.larger;
    	
    	// Pass a CoordRow pointer to the callback, that's all it will need to update
    	g_signal_connect(G_OBJECT(smallerButton), "clicked", G_CALLBACK(onClickSmaller), &row);
    	g_signal_connect(G_OBJECT(largerButton), "clicked", G_CALLBACK(onClickLarger), &row);
    }
    
    // Create the title label (bold font)
	_tesselation.title = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_TESSELATION + "</span>"
    );
    gtk_misc_set_padding(GTK_MISC(_tesselation.title), 0, 2);
    gtk_box_pack_start(GTK_BOX(dialogVBox), _tesselation.title, false, false, 0);
    
    // Setup the table with default spacings
	_tesselation.table = GTK_TABLE(gtk_table_new(3, 2, false));
    gtk_table_set_col_spacings(_tesselation.table, 12);
    gtk_table_set_row_spacings(_tesselation.table, 6);
    
    // Pack it into an alignment so that it is indented
	GtkWidget* tessAlignment = gtkutil::LeftAlignment(GTK_WIDGET(_tesselation.table), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(tessAlignment), false, false, 0);
	
	// Tesselation checkbox
	_tesselation.fixed = gtk_check_button_new_with_label(LABEL_FIXED.c_str());
	g_signal_connect(G_OBJECT(_tesselation.fixed), "toggled", G_CALLBACK(onFixedTessChange), this);
	gtk_table_attach_defaults(_tesselation.table, _tesselation.fixed, 0, 2, 0, 1);
	
	// Tesselation entry fields
	_tesselation.horiz = gtk_spin_button_new_with_range(TESS_MIN, TESS_MAX, 1.0f);
	_tesselation.vert = gtk_spin_button_new_with_range(TESS_MIN, TESS_MAX, 1.0f);
	
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_tesselation.horiz), 0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_tesselation.vert), 0);
	
	g_signal_connect(G_OBJECT(_tesselation.horiz), "changed", G_CALLBACK(onTessChange), this);
	g_signal_connect(G_OBJECT(_tesselation.vert), "changed", G_CALLBACK(onTessChange), this);
	
	gtk_widget_set_size_request(_tesselation.horiz, 100, -1);
	gtk_widget_set_size_request(_tesselation.vert, 100, -1);
	
	_tesselation.horizLabel = gtkutil::LeftAlignedLabel(LABEL_SUBDIVISION_X);
	_tesselation.vertLabel = gtkutil::LeftAlignedLabel(LABEL_SUBDIVISION_Y);
	
	gtk_table_attach_defaults(_tesselation.table, _tesselation.horizLabel, 0, 1, 1, 2);
	gtk_table_attach_defaults(_tesselation.table, _tesselation.horiz, 1, 2, 1, 2);
	
	gtk_table_attach_defaults(_tesselation.table, _tesselation.vertLabel, 0, 1, 2, 3);
	gtk_table_attach_defaults(_tesselation.table, _tesselation.vert, 1, 2, 2, 3);
}

PatchInspector::CoordRow PatchInspector::createCoordRow(
	const std::string& label, GtkTable* table, int row) 
{
	CoordRow coordRow;
	
	coordRow.hbox = gtk_hbox_new(false, 6);
		
	// Create the label
	coordRow.label = gtkutil::LeftAlignedLabel(label);
	gtk_table_attach_defaults(table, coordRow.label, 0, 1, row, row+1);
		
	// Create the entry field
	coordRow.value = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(coordRow.value), 7);
	
	coordRow.valueChangedHandler = 
		g_signal_connect(G_OBJECT(coordRow.value), "changed", G_CALLBACK(onCoordChange), this);
	
	gtk_box_pack_start(GTK_BOX(coordRow.hbox), coordRow.value, true, true, 0);
	
	{
		GtkWidget* hbox = gtk_hbox_new(true, 0);
		
		coordRow.smaller = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalRadiant().getLocalPixbuf("arrow_left.png"))
		);
		gtk_widget_set_size_request(*coordRow.smaller, 15, 24);
		gtk_box_pack_start(GTK_BOX(hbox), *coordRow.smaller, false, false, 0);
		
		coordRow.larger = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalRadiant().getLocalPixbuf("arrow_right.png"))
		);
		gtk_widget_set_size_request(*coordRow.larger, 15, 24);
		gtk_box_pack_start(GTK_BOX(hbox), *coordRow.larger, false, false, 0); 
		
		gtk_box_pack_start(GTK_BOX(coordRow.hbox), hbox, false, false, 0);
	}
	
	// Create the label
	coordRow.steplabel = gtkutil::LeftAlignedLabel(LABEL_STEP); 
	gtk_box_pack_start(GTK_BOX(coordRow.hbox), coordRow.steplabel, false, false, 0);
	
	// Create the entry field
	coordRow.step = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(coordRow.step), 5);
	g_signal_connect(G_OBJECT(coordRow.step), "changed", G_CALLBACK(onStepChanged), this);
	
	gtk_box_pack_start(GTK_BOX(coordRow.hbox), coordRow.step, false, false, 0);
		
	// Pack the hbox into the table
	gtk_table_attach_defaults(table, coordRow.hbox, 1, 2, row, row+1);
	
	// Return the filled structure
	return coordRow;
}

void PatchInspector::update() {
	_updateActive = true;
	
	if (_patch != NULL) {
		// Load the "fixed tesselation" value
		bool fixed = _patch->subdivionsFixed();
		
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_tesselation.fixed), 
			fixed
		);
		
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(_tesselation.horiz),
			_patch->getSubdivisions()[0]
		);
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(_tesselation.vert),
			_patch->getSubdivisions()[1]
		);
		
		gtk_widget_set_sensitive(_tesselation.horiz, fixed);
		gtk_widget_set_sensitive(_tesselation.vert, fixed);
		gtk_widget_set_sensitive(_tesselation.vertLabel, fixed);
		gtk_widget_set_sensitive(_tesselation.horizLabel, fixed);
		
		// Load the data from the vertex
		loadControlVertex();
	}
	
	_updateActive = false;
}

void PatchInspector::loadControlVertex() {
	if (_patch != NULL) {
		int row = strToInt(gtk_combo_box_get_active_text(GTK_COMBO_BOX(_vertexChooser.rowCombo)));
		int col = strToInt(gtk_combo_box_get_active_text(GTK_COMBO_BOX(_vertexChooser.colCombo)));
		
		// Retrieve the controlvertex
		const PatchControl& ctrl = _patch->ctrlAt(row, col);
		
		_updateActive = true;
		
		gtk_entry_set_text(GTK_ENTRY(_coords["x"].value), floatToStr(ctrl.m_vertex[0]).c_str());
		gtk_entry_set_text(GTK_ENTRY(_coords["y"].value), floatToStr(ctrl.m_vertex[1]).c_str());
		gtk_entry_set_text(GTK_ENTRY(_coords["z"].value), floatToStr(ctrl.m_vertex[2]).c_str());
		gtk_entry_set_text(GTK_ENTRY(_coords["s"].value), floatToStr(ctrl.m_texcoord[0]).c_str());
		gtk_entry_set_text(GTK_ENTRY(_coords["t"].value), floatToStr(ctrl.m_texcoord[1]).c_str());
		
		_updateActive = false;
	}
}

void PatchInspector::toggleWindow() {
	if (isVisible())
		hide();
	else
		show();
}

// Pre-hide callback
void PatchInspector::_preHide() {
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void PatchInspector::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();
	// Update the widget values
	update();
}

void PatchInspector::selectionChanged(const scene::INodePtr& node, bool isComponent) {
	rescanSelection();
}

void PatchInspector::rescanSelection() {
	// Check if there is one distinct patch selected
	bool sensitive = (_selectionInfo.patchCount == 1);

	gtk_widget_set_sensitive(GTK_WIDGET(_vertexChooser.table), sensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(_vertexChooser.title), sensitive);
	
	gtk_widget_set_sensitive(_tesselation.title, sensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(_tesselation.table), sensitive);
	
	gtk_widget_set_sensitive(_coordsLabel, sensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(_coordsTable), sensitive);

	_updateActive = true;
	
	// Remove all the items from the combo boxes
	for (std::size_t i = 0; i < _patchRows; i++) {
		gtk_combo_box_remove_text(GTK_COMBO_BOX(_vertexChooser.rowCombo), 0);
	}
	
	for (std::size_t i = 0; i < _patchCols; i++) {
		gtk_combo_box_remove_text(GTK_COMBO_BOX(_vertexChooser.colCombo), 0);
	}
	
	_updateActive = false;
	
	_patch = NULL;
	_patchRows = 0;
	_patchCols = 0;
	
	if (sensitive) {
		PatchPtrVector list = selection::algorithm::getSelectedPatches();
		
		if (list.size() > 0) {
			_patch = &(list[0]->getPatch());
			_patchRows = _patch->getHeight();
			_patchCols = _patch->getWidth();
			
			_updateActive = true;
			
			for (std::size_t i = 0; i < _patchRows; i++) {
				gtk_combo_box_append_text(
					GTK_COMBO_BOX(_vertexChooser.rowCombo), 
					sizetToStr(i).c_str()
				);
			}

			gtk_combo_box_set_active(
				GTK_COMBO_BOX(_vertexChooser.rowCombo), 
				0
			);

			for (std::size_t i = 0; i < _patchCols; i++) {
				gtk_combo_box_append_text(
					GTK_COMBO_BOX(_vertexChooser.colCombo), 
					sizetToStr(i).c_str()
				);
			}
			gtk_combo_box_set_active(
				GTK_COMBO_BOX(_vertexChooser.colCombo), 
				0
			);
			
			_updateActive = false;
		}
	}
	
	update();
}

void PatchInspector::emitCoords() {
	// Save the coords into the patch
	if (_patch != NULL) {
		int row = strToInt(gtk_combo_box_get_active_text(GTK_COMBO_BOX(_vertexChooser.rowCombo)));
		int col = strToInt(gtk_combo_box_get_active_text(GTK_COMBO_BOX(_vertexChooser.colCombo)));
		
		// Retrieve the controlvertex
		PatchControl& ctrl = _patch->ctrlAt(row, col);
		
		ctrl.m_vertex[0] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["x"].value)));
		ctrl.m_vertex[1] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["y"].value)));
		ctrl.m_vertex[2] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["z"].value)));
		
		ctrl.m_texcoord[0] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["s"].value)));
		ctrl.m_texcoord[1] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["t"].value)));
		
		_patch->controlPointsChanged();
	}
}

void PatchInspector::emitTesselation() {
	// Save the setting into the patch
	if (_patch != NULL) {
		UndoableCommand setFixedTessCmd("patchSetFixedTesselation");
		
		BasicVector2<unsigned int> tess(
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(_tesselation.horiz)),
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(_tesselation.vert))
		);
		
		bool fixed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_tesselation.fixed)) ? true : false;
		
		_patch->setFixedSubdivisions(fixed, tess);
		
		gtk_widget_set_sensitive(_tesselation.horiz, fixed);
		gtk_widget_set_sensitive(_tesselation.vert, fixed);
		gtk_widget_set_sensitive(_tesselation.vertLabel, fixed);
		gtk_widget_set_sensitive(_tesselation.horizLabel, fixed);
	}
}

void PatchInspector::saveToRegistry() {
	// Pass the call to the RegistryConnector
	_connector.exportValues();
}

void PatchInspector::onCoordChange(GtkEditable* editable, PatchInspector* self) {
	if (self->_updateActive) {
		return;
	}
	self->emitCoords();
}

void PatchInspector::onStepChanged(GtkEditable* editable, PatchInspector* self) {
	// Tell the class instance to save its contents into the registry
	self->saveToRegistry();
} 

void PatchInspector::onTessChange(GtkEditable* editable, PatchInspector* self) {
	if (self->_updateActive) {
		return;
	}
	self->emitTesselation();
}

void PatchInspector::onFixedTessChange(GtkWidget* checkButton, PatchInspector* self) {
	if (self->_updateActive) {
		return;
	}
	self->emitTesselation();
}

void PatchInspector::onComboBoxChange(GtkWidget* combo, PatchInspector* self) {
	if (self->_updateActive) {
		return;
	}
	// Load the according patch row/col vertex
	self->loadControlVertex();
}

void PatchInspector::onClickLarger(GtkWidget* button, CoordRow* row) {
	// Get the current value and the step increment
	float value = strToFloat(gtk_entry_get_text(GTK_ENTRY(row->value)));
	float step = strToFloat(gtk_entry_get_text(GTK_ENTRY(row->step)));
	
	// This triggers the onCoordChange callback method
	gtk_entry_set_text(GTK_ENTRY(row->value), floatToStr(value + step).c_str());
}

void PatchInspector::onClickSmaller(GtkWidget* button, CoordRow* row) {
	// Get the current value and the step increment
	float value = strToFloat(gtk_entry_get_text(GTK_ENTRY(row->value)));
	float step = strToFloat(gtk_entry_get_text(GTK_ENTRY(row->step)));
	
	// This triggers the onCoordChange callback method
	gtk_entry_set_text(GTK_ENTRY(row->value), floatToStr(value - step).c_str());
}

// static command target
void PatchInspector::toggle() {
	Instance().toggleWindow();
}

} // namespace ui
