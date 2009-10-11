#include "SurfaceInspector.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "ieventmanager.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/ControlButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/dialog.h"
#include "gtkutil/SerialisableWidgets.h"

#include "selectionlib.h"
#include "math/FloatTools.h"
#include "string/string.h"

#include "textool/TexTool.h"
#include "ui/patch/PatchInspector.h"
#include "brush/TextureProjection.h"
#include "selection/algorithm/Primitives.h"
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
		const char* FOLDER_ICON = "folder16.png";
		const char* LABEL_STEP = "Step:";
		
		const char* LABEL_FIT_TEXTURE = "Fit Texture:";
		const char* LABEL_FIT = "Fit";
		
		const char* LABEL_FLIP_TEXTURE = "Flip Texture:";
		const char* LABEL_FLIPX = "Flip Horizontal";
		const char* LABEL_FLIPY = "Flip Vertical";
				
		const char* LABEL_APPLY_TEXTURE = "Modify Texture:";
		const char* LABEL_NATURAL = "Natural";
		const char* LABEL_NORMALISE = "Normalise";
		
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
		
		const double MAX_FLOAT_RESOLUTION = 1.0E-5;
	}

SurfaceInspector::SurfaceInspector() 
: gtkutil::PersistentTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow(), true),
  _callbackActive(false),
  _selectionInfo(GlobalSelectionSystem().getSelectionInfo())
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(
		GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG
	);
	
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Connect the defaultTexScale and texLockButton widgets to "their" registry keys
	using namespace gtkutil;

	_connector.addObject(
      RKEY_DEFAULT_TEXTURE_SCALE,
      SerialisableWidgetWrapperPtr(
         new SerialisableSpinButton(_defaultTexScale)
      )
	);
	_connector.addObject(
      RKEY_ENABLE_TEXTURE_LOCK,
      SerialisableWidgetWrapperPtr(
         new SerialisableToggleButton(_texLockButton)
      )
	);
	
	// Connect the step values to the according registry values
	_connector.addObject(
      RKEY_HSHIFT_STEP,
      SerialisableWidgetWrapperPtr(
         new SerialisableTextEntry(_manipulators[HSHIFT].step)
      )
	);
	_connector.addObject(
      RKEY_VSHIFT_STEP,
      SerialisableWidgetWrapperPtr(
         new SerialisableTextEntry(_manipulators[VSHIFT].step)
      )
	);
	_connector.addObject(
      RKEY_HSCALE_STEP,
      SerialisableWidgetWrapperPtr(
         new SerialisableTextEntry(_manipulators[HSCALE].step)
      )
	);
	_connector.addObject(
      RKEY_VSCALE_STEP,
      SerialisableWidgetWrapperPtr(
         new SerialisableTextEntry(_manipulators[VSCALE].step)
      )
	);
	_connector.addObject(
      RKEY_ROTATION_STEP,
      SerialisableWidgetWrapperPtr(
         new SerialisableTextEntry(_manipulators[ROTATION].step)
      )
	);
	
	// Load the values from the Registry
	_connector.importValues();
	
	// Be notified upon key changes
	GlobalRegistry().addKeyObserver(this, RKEY_ENABLE_TEXTURE_LOCK);
	GlobalRegistry().addKeyObserver(this, RKEY_DEFAULT_TEXTURE_SCALE);
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Update the widget status
	update();
	
	// Get the relevant Events from the Manager and connect the widgets
	connectEvents();
	
	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
}

SurfaceInspectorPtr& SurfaceInspector::InstancePtr() {
	static SurfaceInspectorPtr _instancePtr;
	
	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = SurfaceInspectorPtr(new SurfaceInspector);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}
	
	return _instancePtr;
}

void SurfaceInspector::onRadiantShutdown()
{
	globalOutputStream() << "SurfaceInspector shutting down.\n";

	if (isVisible())
	{
		hide();
	}

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));

	// Destroy the window (after it has been disconnected from the Eventmanager)
	destroy();
}

void SurfaceInspector::connectEvents() {
	// Connect the ToggleTexLock item to the according command
	GlobalEventManager().findEvent("TogTexLock")->connectWidget(_texLockButton);
	GlobalEventManager().findEvent("FlipTextureX")->connectWidget(_flipTexture.flipX);
	GlobalEventManager().findEvent("FlipTextureY")->connectWidget(_flipTexture.flipY);
	GlobalEventManager().findEvent("TextureNatural")->connectWidget(_applyTex.natural);
	GlobalEventManager().findEvent("NormaliseTexture")->connectWidget(_applyTex.normalise);
	
	GlobalEventManager().findEvent("TexShiftLeft")->connectWidget(*_manipulators[HSHIFT].smaller);
	GlobalEventManager().findEvent("TexShiftRight")->connectWidget(*_manipulators[HSHIFT].larger);
	GlobalEventManager().findEvent("TexShiftUp")->connectWidget(*_manipulators[VSHIFT].larger);
	GlobalEventManager().findEvent("TexShiftDown")->connectWidget(*_manipulators[VSHIFT].smaller);
	GlobalEventManager().findEvent("TexScaleLeft")->connectWidget(*_manipulators[HSCALE].smaller);
	GlobalEventManager().findEvent("TexScaleRight")->connectWidget(*_manipulators[HSCALE].larger);
	GlobalEventManager().findEvent("TexScaleUp")->connectWidget(*_manipulators[VSCALE].larger);
	GlobalEventManager().findEvent("TexScaleDown")->connectWidget(*_manipulators[VSCALE].smaller);
	GlobalEventManager().findEvent("TexRotateClock")->connectWidget(*_manipulators[ROTATION].larger);
	GlobalEventManager().findEvent("TexRotateCounter")->connectWidget(*_manipulators[ROTATION].smaller);
	
	// Be sure to connect these signals after the buttons are connected 
	// to the events, so that the update() call gets invoked after the actual event has been fired.
	g_signal_connect(G_OBJECT(_fitTexture.button), "clicked", G_CALLBACK(onFit), this);
	g_signal_connect(G_OBJECT(_flipTexture.flipX), "clicked", G_CALLBACK(doUpdate), this);
	g_signal_connect(G_OBJECT(_flipTexture.flipY), "clicked", G_CALLBACK(doUpdate), this);
	g_signal_connect(G_OBJECT(_applyTex.natural), "clicked", G_CALLBACK(doUpdate), this);
	g_signal_connect(G_OBJECT(_applyTex.normalise), "clicked", G_CALLBACK(doUpdate), this);
	g_signal_connect(G_OBJECT(_defaultTexScale), "value-changed", G_CALLBACK(onDefaultScaleChanged), this);
	
	for (ManipulatorMap::iterator i = _manipulators.begin(); i != _manipulators.end(); i++) {
		GtkWidget* smaller = *(i->second.smaller);
		GtkWidget* larger = *(i->second.larger);
		
		g_signal_connect(G_OBJECT(smaller), "clicked", G_CALLBACK(doUpdate), this);
		g_signal_connect(G_OBJECT(larger), "clicked", G_CALLBACK(doUpdate), this);
	}
}

void SurfaceInspector::toggleWindow() {
	// Toggle the dialog visibility
	if (isVisible())
		hide();
	else
		show();
}

void SurfaceInspector::keyChanged(const std::string& key, const std::string& val) 
{
	// Avoid callback loops
	if (_callbackActive) { 
		return;
	}
	
	_callbackActive = true;
	
	// Disable this event to prevent double-firing
	GlobalEventManager().findEvent("TogTexLock")->setEnabled(false);
	
	// Tell the registryconnector to import the values from the Registry
	_connector.importValues();
	
	// Re-enable the event
	GlobalEventManager().findEvent("TogTexLock")->setEnabled(true);
	
	_callbackActive = false;
}

void SurfaceInspector::populateWindow() {
	// Create the overall vbox
	GtkWidget* dialogVBox = gtk_vbox_new(false, 6);
	gtk_container_add(GTK_CONTAINER(getWindow()), dialogVBox);
	
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
	g_signal_connect(G_OBJECT(_shaderEntry), "key-press-event", G_CALLBACK(onKeyPress), this);
	
	// Create the icon button to open the ShaderChooser
	_selectShaderButton = gtkutil::IconTextButton("", GlobalRadiant().getLocalPixbuf(FOLDER_ICON), false);
	// Override the size request
	gtk_widget_set_size_request(_selectShaderButton, -1, -1); 
	g_signal_connect(G_OBJECT(_selectShaderButton), "clicked", G_CALLBACK(onShaderSelect), this);
	
	GtkWidget* hbox = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _shaderEntry, true, true, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _selectShaderButton, false, false, 0);
	
	gtk_table_attach_defaults(table, hbox, 1, 2, 0, 1);
	
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
	
	_fitTexture.widthAdj = gtk_adjustment_new(1.0f, 0.0f, 1000.0f, 1.0f, 1.0f, 0);
	_fitTexture.heightAdj = gtk_adjustment_new(1.0f, 0.0f, 1000.0f, 1.0f, 1.0f, 0);
	
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
	_applyTex.normalise = gtk_button_new_with_label(LABEL_NORMALISE);
	gtk_box_pack_start(GTK_BOX(_applyTex.hbox), _applyTex.natural, true, true, 0);
	gtk_box_pack_start(GTK_BOX(_applyTex.hbox), _applyTex.normalise, true, true, 0);
	
	gtk_table_attach_defaults(operTable, _applyTex.hbox, 1, 2, 2, 3);
	
	// Default Scale
	GtkWidget* defaultScaleLabel = gtkutil::LeftAlignedLabel(LABEL_DEFAULT_SCALE);
	gtk_table_attach_defaults(operTable, defaultScaleLabel, 0, 1, 3, 4);
	
	GtkWidget* hbox2 = gtk_hbox_new(true, 6);
	 
	// Create the default texture scale spinner
	GtkObject* defaultAdj = gtk_adjustment_new(
		GlobalRegistry().getFloat(RKEY_DEFAULT_TEXTURE_SCALE), 
		0.0f, 1000.0f, 0.1f, 0.1f, 0
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
	
	manipRow.valueChangedHandler = 
		g_signal_connect(G_OBJECT(manipRow.value), "key-press-event", G_CALLBACK(onValueKeyPress), this);
	
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.value, true, true, 0);
	
	if (vertical) {
		GtkWidget* vbox = gtk_vbox_new(true, 0);
		
		manipRow.larger = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalRadiant().getLocalPixbuf("arrow_up.png"))
		);
		gtk_widget_set_size_request(*manipRow.larger, 30, 12);
		gtk_box_pack_start(GTK_BOX(vbox), *manipRow.larger, false, false, 0);
		
		manipRow.smaller = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalRadiant().getLocalPixbuf("arrow_down.png"))
		);
		gtk_widget_set_size_request(*manipRow.smaller, 30, 12);
		gtk_box_pack_start(GTK_BOX(vbox), *manipRow.smaller, false, false, 0);
		
		gtk_box_pack_start(GTK_BOX(manipRow.hbox), vbox, false, false, 0);
	}
	else {
		GtkWidget* hbox = gtk_hbox_new(true, 0);
		
		manipRow.smaller = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalRadiant().getLocalPixbuf("arrow_left.png"))
		);
		gtk_widget_set_size_request(*manipRow.smaller, 15, 24);
		gtk_box_pack_start(GTK_BOX(hbox), *manipRow.smaller, false, false, 0);
		
		manipRow.larger = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalRadiant().getLocalPixbuf("arrow_right.png"))
		);
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
	return *InstancePtr();
}

void SurfaceInspector::emitShader() {
	
	std::string shaderName = gtk_entry_get_text(GTK_ENTRY(_shaderEntry));
		
	// Apply it to the selection
	selection::algorithm::applyShaderToSelection(shaderName);
	
	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
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
	
	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
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
	texdef._shift[0] = float_snapped(static_cast<double>(texdef._shift[0]), MAX_FLOAT_RESOLUTION);
	texdef._shift[1] = float_snapped(static_cast<double>(texdef._shift[1]), MAX_FLOAT_RESOLUTION);
	texdef._scale[0] = float_snapped(static_cast<double>(texdef._scale[0]), MAX_FLOAT_RESOLUTION);
	texdef._scale[1] = float_snapped(static_cast<double>(texdef._scale[1]), MAX_FLOAT_RESOLUTION);
	texdef._rotate = float_snapped(static_cast<double>(texdef._rotate), MAX_FLOAT_RESOLUTION);
	
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
	
	// If patches or entities are selected, the value entry fields have no meaning
	valueSensitivity = (_selectionInfo.patchCount == 0 && 
						_selectionInfo.totalCount > 0 &&
						_selectionInfo.entityCount == 0 && 
						selection::algorithm::selectedFaceCount() == 1);
	
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
	
	// The natural/normalise widget sensitivity
	gtk_widget_set_sensitive(_applyTex.hbox, applySensitivity);
	gtk_widget_set_sensitive(_applyTex.label, applySensitivity);
	
	std::string selectedShader = selection::algorithm::getShaderFromSelection();
	gtk_entry_set_text(GTK_ENTRY(_shaderEntry), selectedShader.c_str());
	
	if (valueSensitivity) {
		updateTexDef();
	}
	
	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
	ui::PatchInspector::Instance().update();
}

void SurfaceInspector::postUndo()
{
	// Update the SurfaceInspector after an undo operation
	update();
}

void SurfaceInspector::postRedo()
{
	// Update the SurfaceInspector after a redo operation
	update();
}

// Gets notified upon selection change
void SurfaceInspector::selectionChanged(const scene::INodePtr& node, bool isComponent) {
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
		gtkutil::errorDialog(
			"Both fit values must be > 0.0.", GTK_WINDOW(getWindow())
		);
	}
}

void SurfaceInspector::shaderSelectionChanged(const std::string& shader) {
	emitShader();
}

gboolean SurfaceInspector::onDefaultScaleChanged(GtkSpinButton* spinbutton, SurfaceInspector* self) {
	// Tell the class instance to save its contents into the registry
	self->saveToRegistry();
	return false;
}

void SurfaceInspector::onStepChanged(GtkEditable* editable, SurfaceInspector* self) {
	// Tell the class instance to save its contents into the registry
	self->saveToRegistry();
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

// The GTK keypress callback for the shift/scale/rotation entry fields
gboolean SurfaceInspector::onValueKeyPress(GtkWidget* entry, GdkEventKey* event, SurfaceInspector* self) {
	
	// Check for ESC to deselect all items
	if (event->keyval == GDK_Return) {
		self->emitTexDef();
		// Don't propage the keypress if the Enter could be processed
		gtk_window_set_focus(GTK_WINDOW(self->getWindow()), NULL);
		return true;
	}
	
	return false;
}

// The GTK keypress callback
gboolean SurfaceInspector::onKeyPress(GtkWidget* entry, GdkEventKey* event, SurfaceInspector* self) {
	
	// Check for Enter Key to emit the shader
	if (event->keyval == GDK_Return) {
		self->emitShader();
		// Don't propagate the keypress if the Enter could be processed
		return true;
	}
	
	return false;
}

void SurfaceInspector::onShaderSelect(GtkWidget* button, SurfaceInspector* self) {
	ShaderChooser::ChooserClient* client = self;

	// Instantiate the modal dialog, will block execution
	ShaderChooser chooser(client, GTK_WINDOW(self->getWindow()), self->_shaderEntry);
}

// Static command target to toggle the window
void SurfaceInspector::toggle(const cmd::ArgumentList& args) {
	Instance().toggleWindow();
}

// TransientWindow callbacks
void SurfaceInspector::_preShow()
{
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	GlobalUndoSystem().addObserver(this);

	// Restore the position
	_windowPosition.applyPosition();
	// Import the registry keys 
	_connector.importValues();

	// Re-scan the selection
	update();
}

void SurfaceInspector::_postShow() {
	// Unset the focus widget for this window to avoid the cursor 
	// from jumping into the shader entry field 
	gtk_window_set_focus(GTK_WINDOW(getWindow()), NULL);
}

void SurfaceInspector::_preHide()
{
	// Save the window position, to make sure
	_windowPosition.readPosition();

	GlobalUndoSystem().removeObserver(this);
	GlobalSelectionSystem().removeObserver(this);
}

} // namespace ui
