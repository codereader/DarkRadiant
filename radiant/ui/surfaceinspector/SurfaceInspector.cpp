#include "SurfaceInspector.h"

#include "i18n.h"

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/table.h>
#include <gtkmm/spinbutton.h>

#include <gdk/gdkkeysyms.h>
#include "ieventmanager.h"
#include "itextstream.h"
#include "iuimanager.h"
#include "imainframe.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/ControlButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/dialog/MessageBox.h"

#include "registry/bind.h"
#include "selectionlib.h"
#include "math/FloatTools.h"
#include "string/string.h"

#include "textool/TexTool.h"
#include "ui/patch/PatchInspector.h"
#include "brush/TextureProjection.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Shader.h"

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Surface Inspector");
    const char* const LABEL_PROPERTIES = N_("Texture Properties");
    const char* const LABEL_OPERATIONS = N_("Texture Operations");

    const std::string HSHIFT = "horizshift";
    const std::string VSHIFT = "vertshift";
    const std::string HSCALE = "horizscale";
    const std::string VSCALE = "vertscale";
    const std::string ROTATION = "rotation";

    const char* const LABEL_HSHIFT = N_("Horiz. Shift:");
    const char* const LABEL_VSHIFT = N_("Vert. Shift:");
    const char* const LABEL_HSCALE = N_("Horiz. Scale:");
    const char* const LABEL_VSCALE = N_("Vert. Scale:");
    const char* const LABEL_ROTATION = N_("Rotation:");
    const char* const LABEL_SHADER = N_("Shader:");
    const char* const FOLDER_ICON = "folder16.png";
    const char* const LABEL_STEP = N_("Step:");

    const char* LABEL_FIT_TEXTURE = N_("Fit Texture:");
    const char* LABEL_FIT = N_("Fit");

    const char* LABEL_ALIGN_TEXTURE = N_("Align Texture:");
    const char* LABEL_ALIGN_TOP = N_("Top");
    const char* LABEL_ALIGN_BOTTOM = N_("Bottom");
    const char* LABEL_ALIGN_RIGHT = N_("Right");
    const char* LABEL_ALIGN_LEFT = N_("Left");

    const char* LABEL_FLIP_TEXTURE = N_("Flip Texture:");
    const char* LABEL_FLIPX = N_("Flip Horizontal");
    const char* LABEL_FLIPY = N_("Flip Vertical");

    const char* LABEL_APPLY_TEXTURE = N_("Modify Texture:");
    const char* LABEL_NATURAL = N_("Natural");
    const char* LABEL_NORMALISE = N_("Normalise");

    const char* LABEL_DEFAULT_SCALE = N_("Default Scale:");
    const char* LABEL_TEXTURE_LOCK = N_("Texture Lock");

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
: gtkutil::PersistentTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow(), true),
  _callbackActive(false),
  _selectionInfo(GlobalSelectionSystem().getSelectionInfo())
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create all the widgets and pack them into the window
	populateWindow();

	// Connect the defaultTexScale and texLockButton widgets to "their" registry keys
    registry::bindPropertyToKey(_defaultTexScale->property_value(),
                                RKEY_DEFAULT_TEXTURE_SCALE);
    registry::bindPropertyToKey(_texLockButton->property_active(), 
                                RKEY_ENABLE_TEXTURE_LOCK);

	// Connect the step values to the according registry values
    registry::bindPropertyToKey(_manipulators[HSHIFT].stepEntry->property_text(),
                                RKEY_HSHIFT_STEP);
    registry::bindPropertyToKey(_manipulators[VSHIFT].stepEntry->property_text(),
                                RKEY_VSHIFT_STEP);
    registry::bindPropertyToKey(_manipulators[HSCALE].stepEntry->property_text(),
                                RKEY_HSCALE_STEP);
    registry::bindPropertyToKey(_manipulators[VSCALE].stepEntry->property_text(),
                                RKEY_VSCALE_STEP);
    registry::bindPropertyToKey(_manipulators[ROTATION].stepEntry->property_text(),
                                RKEY_ROTATION_STEP);

	// Be notified upon key changes
	GlobalRegistry().signalForKey(RKEY_ENABLE_TEXTURE_LOCK).connect(
        sigc::mem_fun(this, &SurfaceInspector::keyChanged)
    );
	GlobalRegistry().signalForKey(RKEY_DEFAULT_TEXTURE_SCALE).connect(
        sigc::mem_fun(this, &SurfaceInspector::keyChanged)
    );

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(this);

	// Update the widget status
	doUpdate();

	// Get the relevant Events from the Manager and connect the widgets
	connectEvents();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

SurfaceInspectorPtr& SurfaceInspector::InstancePtr()
{
	static SurfaceInspectorPtr _instancePtr;
	return _instancePtr;
}

void SurfaceInspector::onRadiantShutdown()
{
	rMessage() << "SurfaceInspector shutting down.\n";

	if (is_visible())
	{
		hide();
	}

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(this);

	// Destroy the window (after it has been disconnected from the Eventmanager)
	destroy();

	InstancePtr().reset();
}

void SurfaceInspector::connectEvents()
{
	// Connect the ToggleTexLock item to the according command
	GlobalEventManager().findEvent("TogTexLock")->connectWidget(_texLockButton);
	GlobalEventManager().findEvent("FlipTextureX")->connectWidget(_flipTexture.flipX);
	GlobalEventManager().findEvent("FlipTextureY")->connectWidget(_flipTexture.flipY);
	GlobalEventManager().findEvent("TextureNatural")->connectWidget(_applyTex.natural);
	GlobalEventManager().findEvent("NormaliseTexture")->connectWidget(_applyTex.normalise);

	GlobalEventManager().findEvent("TexAlignTop")->connectWidget(_alignTexture.top);
	GlobalEventManager().findEvent("TexAlignBottom")->connectWidget(_alignTexture.bottom);
	GlobalEventManager().findEvent("TexAlignRight")->connectWidget(_alignTexture.right);
	GlobalEventManager().findEvent("TexAlignLeft")->connectWidget(_alignTexture.left);

	GlobalEventManager().findEvent("TexShiftLeft")->connectWidget(_manipulators[HSHIFT].smaller);
	GlobalEventManager().findEvent("TexShiftRight")->connectWidget(_manipulators[HSHIFT].larger);
	GlobalEventManager().findEvent("TexShiftUp")->connectWidget(_manipulators[VSHIFT].larger);
	GlobalEventManager().findEvent("TexShiftDown")->connectWidget(_manipulators[VSHIFT].smaller);
	GlobalEventManager().findEvent("TexScaleLeft")->connectWidget(_manipulators[HSCALE].smaller);
	GlobalEventManager().findEvent("TexScaleRight")->connectWidget(_manipulators[HSCALE].larger);
	GlobalEventManager().findEvent("TexScaleUp")->connectWidget(_manipulators[VSCALE].larger);
	GlobalEventManager().findEvent("TexScaleDown")->connectWidget(_manipulators[VSCALE].smaller);
	GlobalEventManager().findEvent("TexRotateClock")->connectWidget(_manipulators[ROTATION].larger);
	GlobalEventManager().findEvent("TexRotateCounter")->connectWidget(_manipulators[ROTATION].smaller);

	// Be sure to connect these signals after the buttons are connected
	// to the events, so that the doUpdate() call gets invoked after the actual event has been fired.
	_fitTexture.button->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::onFit));

	_flipTexture.flipX->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
	_flipTexture.flipY->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
	_alignTexture.top->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
	_alignTexture.bottom->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
	_alignTexture.right->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
	_alignTexture.left->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
	_applyTex.natural->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
	_applyTex.normalise->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));

	for (ManipulatorMap::iterator i = _manipulators.begin(); i != _manipulators.end(); ++i)
	{
		i->second.smaller->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
		i->second.larger->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::doUpdate));
	}
}

void SurfaceInspector::keyChanged()
{
	// Avoid callback loops
	if (_callbackActive) {
		return;
	}

	_callbackActive = true;

	// Disable this event to prevent double-firing
	GlobalEventManager().findEvent("TogTexLock")->setEnabled(false);

	// Re-enable the event
	GlobalEventManager().findEvent("TogTexLock")->setEnabled(true);

	_callbackActive = false;
}

void SurfaceInspector::populateWindow()
{
	// Create the overall vbox
	Gtk::VBox* dialogVBox = Gtk::manage(new Gtk::VBox(false, 6));
	add(*dialogVBox);

	// Create the title label (bold font)
	Gtk::Label* topLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + _(LABEL_PROPERTIES) + "</span>"
    ));
	dialogVBox->pack_start(*topLabel, true, true, 0);

    // Setup the table with default spacings
	Gtk::Table* table = Gtk::manage(new Gtk::Table(6, 2, false));
    table->set_col_spacings(12);
    table->set_row_spacings(6);

    // Pack it into an alignment so that it is indented
	Gtk::Widget* alignment = Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 1.0));
	dialogVBox->pack_start(*alignment, true, true, 0);

	// Create the entry field and pack it into the first table row
	Gtk::Label* shaderLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_SHADER)));
	table->attach(*shaderLabel, 0, 1, 0, 1);

	_shaderEntry = Gtk::manage(new Gtk::Entry);
	_shaderEntry->signal_key_press_event().connect(sigc::mem_fun(*this, &SurfaceInspector::onKeyPress), false);

	// Create the icon button to open the ShaderChooser
	_selectShaderButton = Gtk::manage(
		new gtkutil::IconTextButton("", GlobalUIManager().getLocalPixbuf(FOLDER_ICON))
	);

	// Override the size request
	_selectShaderButton->set_size_request(-1, -1);
	_selectShaderButton->signal_clicked().connect(sigc::mem_fun(*this, &SurfaceInspector::onShaderSelect));

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 0));
	hbox->pack_start(*_shaderEntry, true, true, 0);
	hbox->pack_start(*_selectShaderButton, false, false, 0);

	table->attach(*hbox, 1, 2, 0, 1);

	// Populate the table with the according widgets
	_manipulators[HSHIFT] = createManipulatorRow(_(LABEL_HSHIFT), *table, 1, false);
	_manipulators[VSHIFT] = createManipulatorRow(_(LABEL_VSHIFT), *table, 2, true);
	_manipulators[HSCALE] = createManipulatorRow(_(LABEL_HSCALE), *table, 3, false);
	_manipulators[VSCALE] = createManipulatorRow(_(LABEL_VSCALE), *table, 4, true);
	_manipulators[ROTATION] = createManipulatorRow(_(LABEL_ROTATION), *table, 5, false);

	// ======================== Texture Operations ====================================

	// Create the texture operations label (bold font)
	Gtk::Label* operLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + _(LABEL_OPERATIONS) + "</span>"
    ));
    operLabel->set_padding(0, 2); // Small spacing to the top/bottom
    dialogVBox->pack_start(*operLabel, true, true, 0);

    // Setup the table with default spacings
	Gtk::Table* operTable = Gtk::manage(new Gtk::Table(5, 2, false));
    operTable->set_col_spacings(12);
    operTable->set_row_spacings(6);

    // Pack this into another alignment
	Gtk::Widget* operAlignment = Gtk::manage(new gtkutil::LeftAlignment(*operTable, 18, 1.0));

    // Pack the table into the dialog
	dialogVBox->pack_start(*operAlignment, true, true, 0);

	// ------------------------ Fit Texture -----------------------------------

	int curLine = 0;

	_fitTexture.hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Create the "Fit Texture" label
	_fitTexture.label = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_FIT_TEXTURE)));
	operTable->attach(*_fitTexture.label, 0, 1, curLine, curLine + 1);

	_fitTexture.widthAdj = Gtk::manage(new Gtk::Adjustment(1.0, 0.0, 1000.0, 1.0, 1.0, 0));
	_fitTexture.heightAdj = Gtk::manage(new Gtk::Adjustment(1.0, 0.0, 1000.0, 1.0, 1.0, 0));

	// Create the width entry field
	_fitTexture.width = Gtk::manage(new Gtk::SpinButton(*_fitTexture.widthAdj, 1.0, 4));
	_fitTexture.width->set_size_request(55, -1);
	_fitTexture.hbox->pack_start(*_fitTexture.width, false, false, 0);

	// Create the "x" label
	Gtk::Label* xLabel = Gtk::manage(new Gtk::Label("x"));
	xLabel->set_alignment(0.5f, 0.5f);
	_fitTexture.hbox->pack_start(*xLabel, false, false, 0);

	// Create the height entry field
	_fitTexture.height = Gtk::manage(new Gtk::SpinButton(*_fitTexture.heightAdj, 1.0, 4));
	_fitTexture.height->set_size_request(55, -1);
	_fitTexture.hbox->pack_start(*_fitTexture.height, false, false, 0);

	_fitTexture.button = Gtk::manage(new Gtk::Button(_(LABEL_FIT)));
	_fitTexture.button->set_size_request(30, -1);
	_fitTexture.hbox->pack_start(*_fitTexture.button, true, true, 0);

	operTable->attach(*_fitTexture.hbox, 1, 2, curLine, curLine + 1);

	// ------------------------ Operation Buttons ------------------------------

	curLine++;

	// Create the "Align Texture" label
	_alignTexture.label = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_ALIGN_TEXTURE)));
	operTable->attach(*_alignTexture.label, 0, 1, curLine, curLine + 1);

	_alignTexture.hbox = Gtk::manage(new Gtk::HBox(true, 6));

	_alignTexture.top = Gtk::manage(new Gtk::Button(_(LABEL_ALIGN_TOP)));
	_alignTexture.bottom = Gtk::manage(new Gtk::Button(_(LABEL_ALIGN_BOTTOM)));
	_alignTexture.left = Gtk::manage(new Gtk::Button(_(LABEL_ALIGN_LEFT)));
	_alignTexture.right = Gtk::manage(new Gtk::Button(_(LABEL_ALIGN_RIGHT)));

	_alignTexture.hbox->pack_start(*_alignTexture.top, true, true, 0);
	_alignTexture.hbox->pack_start(*_alignTexture.bottom, true, true, 0);
	_alignTexture.hbox->pack_start(*_alignTexture.left, true, true, 0);
	_alignTexture.hbox->pack_start(*_alignTexture.right, true, true, 0);

	operTable->attach(*_alignTexture.hbox, 1, 2, curLine, curLine + 1);

	curLine++;

	// Create the "Flip Texture" label
	_flipTexture.label = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_FLIP_TEXTURE)));
	operTable->attach(*_flipTexture.label, 0, 1, curLine, curLine + 1);

	_flipTexture.hbox = Gtk::manage(new Gtk::HBox(true, 6));
	_flipTexture.flipX = Gtk::manage(new Gtk::Button(_(LABEL_FLIPX)));
	_flipTexture.flipY = Gtk::manage(new Gtk::Button(_(LABEL_FLIPY)));
	_flipTexture.hbox->pack_start(*_flipTexture.flipX, true, true, 0);
	_flipTexture.hbox->pack_start(*_flipTexture.flipY, true, true, 0);

	operTable->attach(*_flipTexture.hbox, 1, 2, curLine, curLine + 1);

	curLine++;

	// Create the "Apply Texture" label
	_applyTex.label = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_APPLY_TEXTURE)));
	operTable->attach(*_applyTex.label, 0, 1, curLine, curLine + 1);

	_applyTex.hbox = Gtk::manage(new Gtk::HBox(true, 6));
	_applyTex.natural = Gtk::manage(new Gtk::Button(_(LABEL_NATURAL)));
	_applyTex.normalise = Gtk::manage(new Gtk::Button(_(LABEL_NORMALISE)));
	_applyTex.hbox->pack_start(*_applyTex.natural, true, true, 0);
	_applyTex.hbox->pack_start(*_applyTex.normalise, true, true, 0);

	operTable->attach(*_applyTex.hbox, 1, 2, curLine, curLine + 1);

	curLine++;

	// Default Scale
	Gtk::Label* defaultScaleLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_DEFAULT_SCALE)));
	operTable->attach(*defaultScaleLabel, 0, 1, curLine, curLine + 1);

	Gtk::HBox* hbox2 = Gtk::manage(new Gtk::HBox(true, 6));

	// Create the default texture scale spinner
	Gtk::Adjustment* defaultAdj = Gtk::manage(new Gtk::Adjustment(
		registry::getValue<float>(RKEY_DEFAULT_TEXTURE_SCALE),
		0.0f, 1000.0f, 0.1f, 0.1f, 0)
	);

	_defaultTexScale = Gtk::manage(new Gtk::SpinButton(*defaultAdj, 1.0f, 4));
	_defaultTexScale->set_size_request(55, -1);
	hbox2->pack_start(*_defaultTexScale, true, true, 0);

	// Texture Lock Toggle
	_texLockButton = Gtk::manage(new Gtk::ToggleButton(_(LABEL_TEXTURE_LOCK)));
	hbox2->pack_start(*_texLockButton, true, true, 0);

	operTable->attach(*hbox2, 1, 2, curLine, curLine + 1);
}

SurfaceInspector::ManipulatorRow SurfaceInspector::createManipulatorRow(
	const std::string& label, Gtk::Table& table, int row, bool vertical)
{
	ManipulatorRow manipRow;

	manipRow.hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Create the label
	manipRow.label = Gtk::manage(new gtkutil::LeftAlignedLabel(label));
	table.attach(*manipRow.label, 0, 1, row, row + 1);

	// Create the entry field
	manipRow.value = Gtk::manage(new Gtk::Entry);
	manipRow.value->set_width_chars(7);
	manipRow.value->signal_key_press_event().connect(sigc::mem_fun(*this, &SurfaceInspector::onValueKeyPress), false);

	manipRow.hbox->pack_start(*manipRow.value, true, true, 0);

	if (vertical)
	{
		Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(true, 0));

		manipRow.larger = Gtk::manage(
			new gtkutil::ControlButton(GlobalUIManager().getLocalPixbuf("arrow_up.png"))
		);
		manipRow.larger->set_size_request(30, 12);
		vbox->pack_start(*manipRow.larger, false, false, 0);

		manipRow.smaller = Gtk::manage(
			new gtkutil::ControlButton(GlobalUIManager().getLocalPixbuf("arrow_down.png"))
		);
		manipRow.smaller->set_size_request(30, 12);
		vbox->pack_start(*manipRow.smaller, false, false, 0);

		manipRow.hbox->pack_start(*vbox, false, false, 0);
	}
	else
	{
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(true, 0));

		manipRow.smaller = Gtk::manage(
			new gtkutil::ControlButton(GlobalUIManager().getLocalPixbuf("arrow_left.png"))
		);
		manipRow.smaller->set_size_request(15, 24);
		hbox->pack_start(*manipRow.smaller, false, false, 0);

		manipRow.larger = Gtk::manage(
			new gtkutil::ControlButton(GlobalUIManager().getLocalPixbuf("arrow_right.png"))
		);
		manipRow.larger->set_size_request(15, 24);
		hbox->pack_start(*manipRow.larger, false, false, 0);

		manipRow.hbox->pack_start(*hbox, false, false, 0);
	}

	// Create the label
	manipRow.steplabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_STEP)));
	manipRow.hbox->pack_start(*manipRow.steplabel, false, false, 0);

	// Create the entry field
	manipRow.stepEntry = Gtk::manage(new Gtk::Entry);
	manipRow.stepEntry->set_width_chars(5);

	manipRow.hbox->pack_start(*manipRow.stepEntry, false, false, 0);

	// Pack the hbox into the table
	table.attach(*manipRow.hbox, 1, 2, row, row + 1);

	// Return the filled structure
	return manipRow;
}

SurfaceInspector& SurfaceInspector::Instance()
{
	SurfaceInspectorPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new SurfaceInspector);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &SurfaceInspector::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

void SurfaceInspector::emitShader()
{
	// Apply it to the selection
	selection::algorithm::applyShaderToSelection(_shaderEntry->get_text());

	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
}

void SurfaceInspector::emitTexDef()
{
	TexDef shiftScaleRotate;

	shiftScaleRotate._shift[0] = string::convert<float>(_manipulators[HSHIFT].value->get_text());
	shiftScaleRotate._shift[1] = string::convert<float>(_manipulators[VSHIFT].value->get_text());
	shiftScaleRotate._scale[0] = string::convert<float>(_manipulators[HSCALE].value->get_text());
	shiftScaleRotate._scale[1] = string::convert<float>(_manipulators[VSCALE].value->get_text());
	shiftScaleRotate._rotate = string::convert<float>(_manipulators[ROTATION].value->get_text());

	TextureProjection projection;

	// Construct the BPTexDef out of the TexDef by using the according constructor
	projection.m_brushprimit_texdef = BrushPrimitTexDef(shiftScaleRotate);

	// Apply it to the selection
	selection::algorithm::applyTextureProjectionToFaces(projection);

	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
}

void SurfaceInspector::updateTexDef()
{
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
	_manipulators[HSHIFT].value->set_text(string::to_string(texdef._shift[0]));
	_manipulators[VSHIFT].value->set_text(string::to_string(texdef._shift[1]));

	_manipulators[HSCALE].value->set_text(string::to_string(texdef._scale[0]));
	_manipulators[VSCALE].value->set_text(string::to_string(texdef._scale[1]));

	_manipulators[ROTATION].value->set_text(string::to_string(texdef._rotate));
}

// Public soft update function
void SurfaceInspector::update()
{
    if (InstancePtr())
    {
	    // Request an idle callback to perform the update when GTK is idle
	    Glib::signal_idle().connect_once(
            sigc::mem_fun(*InstancePtr(), &SurfaceInspector::doUpdate)
        );
    }
}

void SurfaceInspector::doUpdate()
{
	bool valueSensitivity = false;
	bool fitSensitivity = (_selectionInfo.totalCount > 0);
	bool flipSensitivity = (_selectionInfo.totalCount > 0);
	bool applySensitivity = (_selectionInfo.totalCount > 0);
	bool alignSensitivity = (_selectionInfo.totalCount > 0);

	// If patches or entities are selected, the value entry fields have no meaning
	valueSensitivity = (_selectionInfo.patchCount == 0 &&
						_selectionInfo.totalCount > 0 &&
						_selectionInfo.entityCount == 0 &&
						selection::algorithm::selectedFaceCount() == 1);

	_manipulators[HSHIFT].value->set_sensitive(valueSensitivity);
	_manipulators[VSHIFT].value->set_sensitive(valueSensitivity);
	_manipulators[HSCALE].value->set_sensitive(valueSensitivity);
	_manipulators[VSCALE].value->set_sensitive(valueSensitivity);
	_manipulators[ROTATION].value->set_sensitive(valueSensitivity);

	// The fit widget sensitivity
	_fitTexture.hbox->set_sensitive(fitSensitivity);
	_fitTexture.label->set_sensitive(fitSensitivity);

	// The align texture widget sensitivity
	_alignTexture.hbox->set_sensitive(alignSensitivity);
	_alignTexture.label->set_sensitive(alignSensitivity);

	// The flip texture widget sensitivity
	_flipTexture.hbox->set_sensitive(flipSensitivity);
	_flipTexture.label->set_sensitive(flipSensitivity);

	// The natural/normalise widget sensitivity
	_applyTex.hbox->set_sensitive(applySensitivity);
	_applyTex.label->set_sensitive(applySensitivity);

	// Current shader name
	_shaderEntry->set_text(selection::algorithm::getShaderFromSelection());

	if (valueSensitivity)
	{
		updateTexDef();
	}

	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
	ui::PatchInspector::Instance().queueUpdate();
}

void SurfaceInspector::postUndo()
{
	// Update the SurfaceInspector after an undo operation
	doUpdate();
}

void SurfaceInspector::postRedo()
{
	// Update the SurfaceInspector after a redo operation
	doUpdate();
}

// Gets notified upon selection change
void SurfaceInspector::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
	doUpdate();
}

void SurfaceInspector::fitTexture()
{
	double repeatX = _fitTexture.width->get_value();
	double repeatY = _fitTexture.height->get_value();

	if (repeatX > 0.0 && repeatY > 0.0)
	{
		selection::algorithm::fitTexture(repeatX, repeatY);
	}
	else
	{
		// Invalid repeatX && repeatY values
		gtkutil::MessageBox::ShowError(_("Both fit values must be > 0."), getRefPtr());
	}
}

void SurfaceInspector::onFit()
{
	// Call the according member method
	fitTexture();
	doUpdate();
}

// The GTK keypress callback for the shift/scale/rotation entry fields
bool SurfaceInspector::onValueKeyPress(GdkEventKey* ev)
{
	// Check for ENTER to emit the texture definition
	if (ev->keyval == GDK_Return)
	{
		emitTexDef();
		// Don't propage the keypress if the Enter could be processed
		unset_focus();
		return true;
	}

	return false;
}

// The GTK keypress callback
bool SurfaceInspector::onKeyPress(GdkEventKey* ev)
{
	// Check for Enter Key to emit the shader
	if (ev->keyval == GDK_Return)
	{
		emitShader();
		// Don't propagate the keypress if the Enter could be processed
		return true;
	}

	return false;
}

void SurfaceInspector::onShaderSelect()
{
	// Instantiate the modal dialog, will block execution
	ShaderChooser chooser(getRefPtr(), _shaderEntry);
    chooser.signal_shaderChanged().connect(
        sigc::mem_fun(this, &SurfaceInspector::emitShader)
    );
    chooser.show();
}

// Static command target to toggle the window
void SurfaceInspector::toggle(const cmd::ArgumentList& args)
{
	Instance().toggleVisibility();
}

// TransientWindow callbacks
void SurfaceInspector::_preShow()
{
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	GlobalUndoSystem().addObserver(this);

	// Restore the position
	_windowPosition.applyPosition();

	// Re-scan the selection
	doUpdate();
}

void SurfaceInspector::_postShow()
{
	// Unset the focus widget for this window to avoid the cursor
	// from jumping into the shader entry field
	unset_focus();
}

void SurfaceInspector::_preHide()
{
	// Save the window position, to make sure
	_windowPosition.readPosition();

	GlobalUndoSystem().removeObserver(this);
	GlobalSelectionSystem().removeObserver(this);
}

} // namespace ui
