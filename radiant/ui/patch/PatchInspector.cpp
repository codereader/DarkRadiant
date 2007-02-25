#include "PatchInspector.h"

#include "iregistry.h"
#include "ieventmanager.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "selectionlib.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"

#include "mainframe.h"
#include "patch/Patch.h"
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
		
		const float TESS_MIN = 1.0f;
		const float TESS_MAX = 32.0f;
		
		const std::string RKEY_ROOT = "user/ui/patch/patchInspector/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	}

PatchInspector::PatchInspector() :
	_selectionInfo(GlobalSelectionSystem().getSelectionInfo()),
	_patchRows(0),
	_patchCols(0),
	_patch(NULL),
	_updateActive(false)
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow(), false);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(_dialog));
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	
	// Update the widget status
	rescanSelection();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
	_windowPosition.applyPosition();
}

void PatchInspector::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(_dialog));
}

PatchInspector& PatchInspector::Instance() {
	// The static instance
	static PatchInspector _inspector;
	
	return _inspector;
}

void PatchInspector::populateWindow() {
	// Create the overall vbox
	GtkWidget* dialogVBox = gtk_vbox_new(false, 6);
	gtk_container_add(GTK_CONTAINER(_dialog), dialogVBox);
	
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
    
    _coords["x"] = createCoordRow("X:", 0);
    _coords["y"] = createCoordRow("Y:", 1);
    _coords["z"] = createCoordRow("Z:", 2);
    _coords["s"] = createCoordRow("S:", 3);
    _coords["t"] = createCoordRow("T:", 4);
    
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
	const std::string& label, int tableRow) 
{
	CoordRow returnValue;
	
	returnValue.label = gtkutil::LeftAlignedLabel(label);
	
	// Create the adjustment for the spin button
	returnValue.adjustment = gtk_adjustment_new(
		0.0f, -65536.0f, 65536.0f, 0.1f, 0.1f, 0.1f
	);
	returnValue.entry = gtk_spin_button_new(GTK_ADJUSTMENT(returnValue.adjustment), 0.1f, 4);
	gtk_widget_set_size_request(returnValue.entry, 100, -1);
	g_signal_connect(G_OBJECT(returnValue.entry), "changed", G_CALLBACK(onCoordChange), this);
	
	gtk_table_attach_defaults(_coordsTable, returnValue.label, 0, 1, tableRow, tableRow+1);
	gtk_table_attach_defaults(_coordsTable, returnValue.entry, 1, 2, tableRow, tableRow+1);
	
	return returnValue;
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
		
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_coords["x"].entry), ctrl.m_vertex[0]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_coords["y"].entry), ctrl.m_vertex[1]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_coords["z"].entry), ctrl.m_vertex[2]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_coords["s"].entry), ctrl.m_texcoord[0]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_coords["t"].entry), ctrl.m_texcoord[1]);
		
		_updateActive = false;
	}
}

void PatchInspector::toggle() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_dialog)) {
		gtkutil::TransientWindow::minimise(_dialog);
		gtk_widget_hide_all(_dialog);
	}
	else {
		gtkutil::TransientWindow::restore(_dialog);
		update();
		gtk_widget_show_all(_dialog);
	}
}

void PatchInspector::selectionChanged(scene::Instance& instance) {
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

	// Remove all the items from the combo boxes
	for (std::size_t i = 0; i < _patchRows; i++) {
		gtk_combo_box_remove_text(GTK_COMBO_BOX(_vertexChooser.rowCombo), 0);
	}
	
	for (std::size_t i = 0; i < _patchCols; i++) {
		gtk_combo_box_remove_text(GTK_COMBO_BOX(_vertexChooser.colCombo), 0);
	}
	
	_patch = NULL;
	_patchRows = 0;
	_patchCols = 0;
	
	if (sensitive) {
		PatchPtrVector list = selection::algorithm::getSelectedPatches();
		
		if (list.size() > 0) {
			_patch = list[0];
			_patchRows = _patch->getHeight();
			_patchCols = _patch->getWidth();
			
			_updateActive = true;
			
			for (std::size_t i = 0; i < _patchRows; i++) {
				gtk_combo_box_append_text(
					GTK_COMBO_BOX(_vertexChooser.rowCombo), 
					intToStr(i).c_str()
				);
			}

			gtk_combo_box_set_active(
				GTK_COMBO_BOX(_vertexChooser.rowCombo), 
				0
			);

			for (std::size_t i = 0; i < _patchCols; i++) {
				gtk_combo_box_append_text(
					GTK_COMBO_BOX(_vertexChooser.colCombo), 
					intToStr(i).c_str()
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
		
		ctrl.m_vertex[0] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["x"].entry)));
		ctrl.m_vertex[1] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["y"].entry)));
		ctrl.m_vertex[2] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["z"].entry)));
		
		ctrl.m_texcoord[0] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["s"].entry)));
		ctrl.m_texcoord[1] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_coords["t"].entry)));
		
		_patch->controlPointsChanged();
	}
}

void PatchInspector::emitTesselation() {
	// Save the setting into the patch
	if (_patch != NULL) {
		BasicVector2<unsigned int> tess(
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(_tesselation.horiz)),
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(_tesselation.vert))
		);
		
		bool fixed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_tesselation.fixed));
		
		_patch->setFixedSubdivisions(fixed, tess);
		
		gtk_widget_set_sensitive(_tesselation.horiz, fixed);
		gtk_widget_set_sensitive(_tesselation.vert, fixed);
		gtk_widget_set_sensitive(_tesselation.vertLabel, fixed);
		gtk_widget_set_sensitive(_tesselation.horizLabel, fixed);
	}
}

gboolean PatchInspector::onDelete(GtkWidget* widget, GdkEvent* event, PatchInspector* self) {
	// Toggle the visibility of the inspector window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}

void PatchInspector::onCoordChange(GtkEditable* editable, PatchInspector* self) {
	if (self->_updateActive) {
		return;
	}
	self->emitCoords();
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

} // namespace ui
