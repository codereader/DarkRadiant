#include "SurfaceInspector.h"

#include <gtk/gtk.h>
#include "ieventmanager.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/ControlButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/dialog.h"
#include "selectionlib.h"
#include "mainframe.h"
#include "math/FloatTools.h"

#include "brush/TextureProjection.h"
#include "selection/algorithm/Shader.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Surface Inspector";
		const std::string LABEL_PROPERTIES = "Texture Properties";
		const std::string LABEL_OPERATIONS = "Texture Operations";
		
		const std::string HSHIFT = "horizshift";
		const std::string VSHIFT = "vertshift";
		const std::string HSCALE = "horizscale";
		const std::string VSCALE = "vertscale";
		const std::string ROTATION = "rotation";
	
		const std::string LABEL_HSHIFT = "Horiz. Shift:";
		const std::string LABEL_VSHIFT = "Vert. Shift:";
		const std::string LABEL_HSCALE = "Horiz. Scale:";
		const std::string LABEL_VSCALE = "Vert. Scale:";
		const std::string LABEL_ROTATION = "Rotation:";
		const char* LABEL_SHADER = "Shader:";
		const char* LABEL_STEP = "Step:";
		
		const char* LABEL_FIT_TEXTURE = "Fit Texture:";
		const char* LABEL_FIT = "Fit";
		
		const char* LABEL_FLIP_TEXTURE = "Flip Texture:";
		const char* LABEL_FLIPX = "Flip Horizontal";
		const char* LABEL_FLIPY = "Flip Vertical";
				
		const char* LABEL_APPLY_TEXTURE = "Apply Texture:";
		const char* LABEL_NATURAL = "Natural";
		const char* LABEL_AXIAL = "Axial";
		
		const char* LABEL_DEFAULT_SCALE = "Default Scale:";
		const char* LABEL_TEXTURE_LOCK = "Texture Lock";
		
		const std::string RKEY_ENABLE_TEXTURE_LOCK = "user/ui/brush/textureLock";
		const std::string RKEY_DEFAULT_TEXTURE_SCALE = "user/ui/textures/defaultTextureScale";

		const std::string RKEY_ROOT = "user/ui/textures/surfaceInspector/";
		const std::string RKEY_HSHIFT_STEP = RKEY_ROOT + "hShiftStep";
		const std::string RKEY_VSHIFT_STEP = RKEY_ROOT + "vShiftStep";
		const std::string RKEY_HSCALE_STEP = RKEY_ROOT + "hScaleStep";
		const std::string RKEY_VSCALE_STEP = RKEY_ROOT + "vScaleStep";
		const std::string RKEY_ROTATION_STEP = RKEY_ROOT + "rotStep";
		
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		
		const float MAX_FLOAT_RESOLUTION = 0.0001f;
	}

SurfaceInspector::SurfaceInspector() :
	_callbackActive(false),
	_selectionInfo(GlobalSelectionSystem().getSelectionInfo())
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow(), false);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Connect the defaultTexScale and texLockButton widgets to "their" registry keys
	_connector.connectGtkObject(GTK_OBJECT(_defaultTexScale), RKEY_DEFAULT_TEXTURE_SCALE);
	_connector.connectGtkObject(GTK_OBJECT(_texLockButton), RKEY_ENABLE_TEXTURE_LOCK);
	
	// Connect the step values to the according registry values
	_connector.connectGtkObject(GTK_OBJECT(_manipulators[HSHIFT].step), RKEY_HSHIFT_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_manipulators[VSHIFT].step), RKEY_VSHIFT_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_manipulators[HSCALE].step), RKEY_HSCALE_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_manipulators[VSCALE].step), RKEY_VSCALE_STEP);
	_connector.connectGtkObject(GTK_OBJECT(_manipulators[ROTATION].step), RKEY_ROTATION_STEP);
	
	// Load the values from the Registry
	_connector.importValues();
	
	// Be notified upon key changes
	GlobalRegistry().addKeyObserver(this, RKEY_ENABLE_TEXTURE_LOCK);
	GlobalRegistry().addKeyObserver(this, RKEY_DEFAULT_TEXTURE_SCALE);
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(_dialog));
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	
	// Update the widget status
	update();
	
	// Get the relevant Events from the Manager and connect the widgets
	connectEvents();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
}

void SurfaceInspector::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(_dialog));
}

void SurfaceInspector::connectEvents() {
	// Connect the ToggleTexLock item to the according command
	GlobalEventManager().findEvent("TogTexLock")->connectWidget(_texLockButton);
	GlobalEventManager().findEvent("FlipTextureX")->connectWidget(_flipTexture.flipX);
	GlobalEventManager().findEvent("FlipTextureY")->connectWidget(_flipTexture.flipY);
	GlobalEventManager().findEvent("TextureNatural")->connectWidget(_applyTex.natural);
	
	GlobalEventManager().findEvent("TexShiftLeft")->connectWidget(*_manipulators[HSHIFT].smaller);
	GlobalEventManager().findEvent("TexShiftRight")->connectWidget(*_manipulators[HSHIFT].larger);
	GlobalEventManager().findEvent("TexShiftUp")->connectWidget(*_manipulators[VSHIFT].larger);
	GlobalEventManager().findEvent("TexShiftDown")->connectWidget(*_manipulators[VSHIFT].smaller);
	GlobalEventManager().findEvent("TexScaleLeft")->connectWidget(*_manipulators[HSCALE].smaller);
	GlobalEventManager().findEvent("TexScaleRight")->connectWidget(*_manipulators[HSCALE].larger);
	GlobalEventManager().findEvent("TexScaleUp")->connectWidget(*_manipulators[VSCALE].larger);
	GlobalEventManager().findEvent("TexScaleDown")->connectWidget(*_manipulators[VSCALE].smaller);
	GlobalEventManager().findEvent("TexRotateClock")->connectWidget(*_manipulators[ROTATION].smaller);
	GlobalEventManager().findEvent("TexRotateCounter")->connectWidget(*_manipulators[ROTATION].larger);
	
	// Be sure to connect these signals after the buttons are connected 
	// to the events, so that the update() call gets invoked after the actual event has been fired.
	g_signal_connect(G_OBJECT(_fitTexture.button), "clicked", G_CALLBACK(onFit), this);
	g_signal_connect(G_OBJECT(_flipTexture.flipX), "clicked", G_CALLBACK(doUpdate), this);
	g_signal_connect(G_OBJECT(_flipTexture.flipY), "clicked", G_CALLBACK(doUpdate), this);
	g_signal_connect(G_OBJECT(_applyTex.natural), "clicked", G_CALLBACK(doUpdate), this);
	
	for (ManipulatorMap::iterator i = _manipulators.begin(); i != _manipulators.end(); i++) {
		GtkWidget* smaller = *(i->second.smaller);
		GtkWidget* larger = *(i->second.larger);
		
		g_signal_connect(G_OBJECT(smaller), "clicked", G_CALLBACK(doUpdate), this);
		g_signal_connect(G_OBJECT(larger), "clicked", G_CALLBACK(doUpdate), this);
	}
}

void SurfaceInspector::toggle() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_dialog)) {
		gtkutil::TransientWindow::minimise(_dialog);
		gtk_widget_hide_all(_dialog);
	}
	else {
		gtkutil::TransientWindow::restore(_dialog);
		_connector.importValues();
		_windowPosition.applyPosition();
		gtk_widget_show_all(_dialog);
	}
}

void SurfaceInspector::keyChanged() {
	// Avoid callback loops
	if (_callbackActive) { 
		return;
	}
	
	_callbackActive = true;
	
	// Tell the registryconnector to import the values from the Registry
	_connector.importValues();
	
	_callbackActive = false;
}

void SurfaceInspector::populateWindow() {
	// Create the overall vbox
	GtkWidget* dialogVBox = gtk_vbox_new(false, 6);
	gtk_container_add(GTK_CONTAINER(_dialog), dialogVBox);
	
	// Create the title label (bold font)
	GtkWidget* topLabel = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_PROPERTIES + "</span>"
    );
    gtk_box_pack_start(GTK_BOX(dialogVBox), topLabel, true, true, 0);
    
    // Setup the table with default spacings
	GtkTable* table = GTK_TABLE(gtk_table_new(6, 2, false));
    gtk_table_set_col_spacings(table, 12);
    gtk_table_set_row_spacings(table, 6);
    
    // Pack it into an alignment so that it is indented
	GtkWidget* alignment = gtkutil::LeftAlignment(GTK_WIDGET(table), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(alignment), true, true, 0);
	
	// Create the entry field and pack it into the first table row
	GtkWidget* shaderLabel = gtkutil::LeftAlignedLabel(LABEL_SHADER);
	gtk_table_attach_defaults(table, shaderLabel, 0, 1, 0, 1);
	
	_shaderEntry = gtk_entry_new();
	//gtk_entry_set_width_chars(GTK_ENTRY(_shaderEntry), 40);
	gtk_table_attach_defaults(table, _shaderEntry, 1, 2, 0, 1);
	
	// Populate the table with the according widgets
	_manipulators[HSHIFT] = createManipulatorRow(LABEL_HSHIFT, table, 1, false);
	_manipulators[VSHIFT] = createManipulatorRow(LABEL_VSHIFT, table, 2, true);
	_manipulators[HSCALE] = createManipulatorRow(LABEL_HSCALE, table, 3, false);
	_manipulators[VSCALE] = createManipulatorRow(LABEL_VSCALE, table, 4, true);
	_manipulators[ROTATION] = createManipulatorRow(LABEL_ROTATION, table, 5, false);
	
	// ======================== Texture Operations ====================================
	
	// Create the texture operations label (bold font)
    GtkWidget* operLabel = gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + LABEL_OPERATIONS + "</span>"
    );
    gtk_misc_set_padding(GTK_MISC(operLabel), 0, 2); // Small spacing to the top/bottom
    gtk_box_pack_start(GTK_BOX(dialogVBox), operLabel, true, true, 0);
    
    // Setup the table with default spacings
	GtkTable* operTable = GTK_TABLE(gtk_table_new(4, 2, false));
    gtk_table_set_col_spacings(operTable, 12);
    gtk_table_set_row_spacings(operTable, 6);
    
    // Pack this into another alignment
	GtkWidget* operAlignment = gtkutil::LeftAlignment(GTK_WIDGET(operTable), 18, 1.0);
    
    // Pack the table into the dialog
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(operAlignment), true, true, 0);
	
	// ------------------------ Fit Texture -----------------------------------
	
	_fitTexture.hbox = gtk_hbox_new(false, 6);
	
	// Create the "Fit Texture" label
	_fitTexture.label = gtkutil::LeftAlignedLabel(LABEL_FIT_TEXTURE);
	gtk_table_attach_defaults(operTable, _fitTexture.label, 0, 1, 0, 1);
	
	_fitTexture.widthAdj = gtk_adjustment_new(1.0f, 0.0f, 1000.0f, 1.0f, 1.0f, 1.0f);
	_fitTexture.heightAdj = gtk_adjustment_new(1.0f, 0.0f, 1000.0f, 1.0f, 1.0f, 1.0f);
	
	// Create the width entry field
	_fitTexture.width = gtk_spin_button_new(GTK_ADJUSTMENT(_fitTexture.widthAdj), 1.0f, 4);
	gtk_widget_set_size_request(_fitTexture.width, 55, -1);
	gtk_box_pack_start(GTK_BOX(_fitTexture.hbox), _fitTexture.width, false, false, 0);
	
	// Create the "x" label
	GtkWidget* xLabel = gtk_label_new("x");
	gtk_misc_set_alignment(GTK_MISC(xLabel), 0.5f, 0.5f);
	gtk_box_pack_start(GTK_BOX(_fitTexture.hbox), xLabel, false, false, 0);
	
	// Create the height entry field
	_fitTexture.height = gtk_spin_button_new(GTK_ADJUSTMENT(_fitTexture.heightAdj), 1.0f, 4);
	gtk_widget_set_size_request(_fitTexture.height, 55, -1);
	gtk_box_pack_start(GTK_BOX(_fitTexture.hbox), _fitTexture.height, false, false, 0);
	
	_fitTexture.button = gtk_button_new_with_label(LABEL_FIT);
	gtk_widget_set_size_request(_fitTexture.button, 30, -1);
	gtk_box_pack_start(GTK_BOX(_fitTexture.hbox), _fitTexture.button, true, true, 0);
		
	gtk_table_attach_defaults(operTable, _fitTexture.hbox, 1, 2, 0, 1);
	
	// ------------------------ Operation Buttons ------------------------------
	
	// Create the "Flip Texture" label
	_flipTexture.label = gtkutil::LeftAlignedLabel(LABEL_FLIP_TEXTURE);
	gtk_table_attach_defaults(operTable, _flipTexture.label, 0, 1, 1, 2);
	
	_flipTexture.hbox = gtk_hbox_new(true, 6); 
	_flipTexture.flipX = gtk_button_new_with_label(LABEL_FLIPX);
	_flipTexture.flipY = gtk_button_new_with_label(LABEL_FLIPY);
	gtk_box_pack_start(GTK_BOX(_flipTexture.hbox), _flipTexture.flipX, true, true, 0);
	gtk_box_pack_start(GTK_BOX(_flipTexture.hbox), _flipTexture.flipY, true, true, 0);
	
	gtk_table_attach_defaults(operTable, _flipTexture.hbox, 1, 2, 1, 2);
	
	// Create the "Apply Texture" label
	_applyTex.label = gtkutil::LeftAlignedLabel(LABEL_APPLY_TEXTURE);
	gtk_table_attach_defaults(operTable, _applyTex.label, 0, 1, 2, 3);
	
	_applyTex.hbox = gtk_hbox_new(true, 6); 
	_applyTex.natural = gtk_button_new_with_label(LABEL_NATURAL);
	_applyTex.axial = gtk_button_new_with_label(LABEL_AXIAL);
	gtk_box_pack_start(GTK_BOX(_applyTex.hbox), _applyTex.natural, true, true, 0);
	gtk_box_pack_start(GTK_BOX(_applyTex.hbox), _applyTex.axial, true, true, 0);
	
	gtk_table_attach_defaults(operTable, _applyTex.hbox, 1, 2, 2, 3);
	
	// Default Scale
	GtkWidget* defaultScaleLabel = gtkutil::LeftAlignedLabel(LABEL_DEFAULT_SCALE);
	gtk_table_attach_defaults(operTable, defaultScaleLabel, 0, 1, 3, 4);
	
	GtkWidget* hbox2 = gtk_hbox_new(true, 6);
	 
	// Create the default texture scale spinner
	GtkObject* defaultAdj = gtk_adjustment_new(
		GlobalRegistry().getFloat(RKEY_DEFAULT_TEXTURE_SCALE), 
		0.0f, 1000.0f, 0.1f, 0.1f, 0.1f
	);
	_defaultTexScale = gtk_spin_button_new(GTK_ADJUSTMENT(defaultAdj), 1.0f, 4);
	gtk_widget_set_size_request(_defaultTexScale, 55, -1);
	gtk_box_pack_start(GTK_BOX(hbox2), _defaultTexScale, true, true, 0);
	
	// Texture Lock Toggle
	_texLockButton = gtk_toggle_button_new_with_label(LABEL_TEXTURE_LOCK); 
	gtk_box_pack_start(GTK_BOX(hbox2), _texLockButton, true, true, 0);
	
	gtk_table_attach_defaults(operTable, hbox2, 1, 2, 3, 4);
}

SurfaceInspector::ManipulatorRow SurfaceInspector::createManipulatorRow(
	const std::string& label, GtkTable* table, int row, bool vertical) 
{
	ManipulatorRow manipRow;
	
	manipRow.hbox = gtk_hbox_new(false, 6);
		
	// Create the label
	manipRow.label = gtkutil::LeftAlignedLabel(label);
	gtk_table_attach_defaults(table, manipRow.label, 0, 1, row, row+1);
		
	// Create the entry field
	manipRow.value = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(manipRow.value), 7);
	g_signal_connect(G_OBJECT(manipRow.value), "changed", G_CALLBACK(onValueChanged), this);
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.value, true, true, 0);
	
	if (vertical) {
		GtkWidget* vbox = gtk_vbox_new(true, 0);
		
		manipRow.larger = ControlButtonPtr(new gtkutil::ControlButton("arrow_up.png"));
		gtk_widget_set_size_request(*manipRow.larger, 30, 12);
		gtk_box_pack_start(GTK_BOX(vbox), *manipRow.larger, false, false, 0);
		
		manipRow.smaller = ControlButtonPtr(new gtkutil::ControlButton("arrow_down.png"));
		gtk_widget_set_size_request(*manipRow.smaller, 30, 12);
		gtk_box_pack_start(GTK_BOX(vbox), *manipRow.smaller, false, false, 0);
		
		gtk_box_pack_start(GTK_BOX(manipRow.hbox), vbox, false, false, 0);
	}
	else {
		GtkWidget* hbox = gtk_hbox_new(true, 0);
		
		manipRow.smaller = ControlButtonPtr(new gtkutil::ControlButton("arrow_left.png"));
		gtk_widget_set_size_request(*manipRow.smaller, 15, 24);
		gtk_box_pack_start(GTK_BOX(hbox), *manipRow.smaller, false, false, 0);
		
		manipRow.larger = ControlButtonPtr(new gtkutil::ControlButton("arrow_right.png"));
		gtk_widget_set_size_request(*manipRow.larger, 15, 24);
		gtk_box_pack_start(GTK_BOX(hbox), *manipRow.larger, false, false, 0);
		
		gtk_box_pack_start(GTK_BOX(manipRow.hbox), hbox, false, false, 0);
	}
	
	// Create the label
	manipRow.steplabel = gtkutil::LeftAlignedLabel(LABEL_STEP); 
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.steplabel, false, false, 0);
	
	// Create the entry field
	manipRow.step = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(manipRow.step), 5);
	g_signal_connect(G_OBJECT(manipRow.step), "changed", G_CALLBACK(onStepChanged), this);
	
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.step, false, false, 0);
		
	// Pack the hbox into the table
	gtk_table_attach_defaults(table, manipRow.hbox, 1, 2, row, row+1);
	
	// Return the filled structure
	return manipRow;
}

SurfaceInspector& SurfaceInspector::Instance() {
	// The static instance
	static SurfaceInspector _inspector;
	
	return _inspector;
}

void SurfaceInspector::emitTexDef() {
	TexDef shiftScaleRotate;

	shiftScaleRotate._shift[0] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_manipulators[HSHIFT].value)));
	shiftScaleRotate._shift[1] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_manipulators[VSHIFT].value)));
	shiftScaleRotate._scale[0] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_manipulators[HSCALE].value)));
	shiftScaleRotate._scale[1] = strToFloat(gtk_entry_get_text(GTK_ENTRY(_manipulators[VSCALE].value)));
	shiftScaleRotate._rotate = strToFloat(gtk_entry_get_text(GTK_ENTRY(_manipulators[ROTATION].value)));
	
	TextureProjection projection;

	// Construct the BPTexDef out of the TexDef by using the according constructor
	projection.m_brushprimit_texdef = BrushPrimitTexDef(shiftScaleRotate);
	
	// Apply it to the selection
	selection::algorithm::applyTextureProjectionToFaces(projection);
}

void SurfaceInspector::updateTexDef() {
	TextureProjection curProjection = selection::algorithm::getSelectedTextureProjection();
	
	// Calculate the "fake" texture properties (shift/scale/rotation) 
	TexDef texdef = curProjection.m_brushprimit_texdef.getFakeTexCoords(); 
	Vector2 shaderDims = selection::algorithm::getSelectedFaceShaderSize(); 
	
	if (shaderDims != Vector2(0,0)) {
		// normalize again to hide the ridiculously high scale values that get created when using texlock
  		texdef._shift[0] = float_mod(texdef._shift[0], shaderDims[0]);
  		texdef._shift[1] = float_mod(texdef._shift[1], shaderDims[1]);
	}
	
	// Snap the floating point variables to the max resolution to avoid things like "1.45e-14"
	texdef._shift[0] = float_snapped(texdef._shift[0], MAX_FLOAT_RESOLUTION);
	texdef._shift[1] = float_snapped(texdef._shift[1], MAX_FLOAT_RESOLUTION);
	texdef._scale[0] = float_snapped(texdef._scale[0], MAX_FLOAT_RESOLUTION);
	texdef._scale[1] = float_snapped(texdef._scale[1], MAX_FLOAT_RESOLUTION);
	texdef._rotate = float_snapped(texdef._rotate, MAX_FLOAT_RESOLUTION);
	
	// Load the values into the widgets
	gtk_entry_set_text(GTK_ENTRY(_manipulators[HSHIFT].value), floatToStr(texdef._shift[0]).c_str());
	gtk_entry_set_text(GTK_ENTRY(_manipulators[VSHIFT].value), floatToStr(texdef._shift[1]).c_str());
	
	gtk_entry_set_text(GTK_ENTRY(_manipulators[HSCALE].value), floatToStr(texdef._scale[0]).c_str());
	gtk_entry_set_text(GTK_ENTRY(_manipulators[VSCALE].value), floatToStr(texdef._scale[1]).c_str());
	
	gtk_entry_set_text(GTK_ENTRY(_manipulators[ROTATION].value), floatToStr(texdef._rotate).c_str());
}

void SurfaceInspector::update() {
	
	bool valueSensitivity = false;
	bool fitSensitivity = (_selectionInfo.totalCount > 0);
	bool flipSensitivity = (_selectionInfo.totalCount > 0);
	bool applySensitivity = (_selectionInfo.totalCount > 0);
	
	// If patches are selected, the value entry fields have no meaning
	valueSensitivity = (_selectionInfo.patchCount == 0 && _selectionInfo.totalCount > 0
						&& selection::algorithm::selectedFaceCount() == 1);
	
	gtk_widget_set_sensitive(_manipulators[HSHIFT].value, valueSensitivity);
	gtk_widget_set_sensitive(_manipulators[VSHIFT].value, valueSensitivity);
	gtk_widget_set_sensitive(_manipulators[HSCALE].value, valueSensitivity);
	gtk_widget_set_sensitive(_manipulators[VSCALE].value, valueSensitivity);
	gtk_widget_set_sensitive(_manipulators[ROTATION].value, valueSensitivity);
	
	// The fit widget sensitivity
	gtk_widget_set_sensitive(_fitTexture.hbox, fitSensitivity);
	gtk_widget_set_sensitive(_fitTexture.label, fitSensitivity);
	
	// The flip texture widget sensitivity
	gtk_widget_set_sensitive(_flipTexture.hbox, flipSensitivity);
	gtk_widget_set_sensitive(_flipTexture.label, flipSensitivity);
	
	// The natural/axial widget sensitivity
	gtk_widget_set_sensitive(_applyTex.hbox, applySensitivity);
	gtk_widget_set_sensitive(_applyTex.label, applySensitivity);
	
	// Temporary disable of Axial button, as it's redundant to "Natural"
	gtk_widget_set_sensitive(_applyTex.axial, false);
	
	std::string selectedShader = selection::algorithm::getShaderFromSelection();
	gtk_entry_set_text(GTK_ENTRY(_shaderEntry), selectedShader.c_str());
	
	if (valueSensitivity) {
		updateTexDef();
	}
}

// Gets notified upon selection change
void SurfaceInspector::selectionChanged() {
	update();
}

void SurfaceInspector::saveToRegistry() {
	// Disable the keyChanged() callback during the update process
	_callbackActive = true;
	
	// Pass the call to the RegistryConnector
	_connector.exportValues();
	
	// Re-enable the callbacks
	_callbackActive = false;
}

void SurfaceInspector::fitTexture() {
	float repeatX = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(_fitTexture.width));
	float repeatY = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(_fitTexture.height));
	
	if (repeatX > 0.0 && repeatY > 0.0) {
		selection::algorithm::fitTexture(repeatX, repeatY);
	}
	else {
		// Invalid repeatX && repeatY values
		gtkutil::errorDialog("Both fit values must be > 0.0.", GTK_WINDOW(_dialog));
	}
}

void SurfaceInspector::onStepChanged(GtkEditable* editable, SurfaceInspector* self) {
	
	// Tell the class instance to save its contents into the registry
	self->saveToRegistry();
} 

gboolean SurfaceInspector::onDelete(GtkWidget* widget, GdkEvent* event, SurfaceInspector* self) {
	// Toggle the visibility of the inspector window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}

gboolean SurfaceInspector::onFit(GtkWidget* widget, SurfaceInspector* self) {
	// Call the according member method
	self->fitTexture();
	self->update();

	return false;
}

gboolean SurfaceInspector::doUpdate(GtkWidget* widget, SurfaceInspector* self) {
	// Update the widgets, everything else is done by the called Event
	self->update();
	return false;
}

void SurfaceInspector::onValueChanged(GtkEditable* editable, SurfaceInspector* self) {
	self->emitTexDef();
}

} // namespace ui
