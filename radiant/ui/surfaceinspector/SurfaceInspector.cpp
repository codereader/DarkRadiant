#include "SurfaceInspector.h"

#include "i18n.h"

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/table.h>
#include <gtkmm/spinbutton.h>

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/artprov.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>

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

SurfaceInspector::SurfaceInspector() : 
	wxutil::TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
	_callbackActive(false)
{

#if 0
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
#endif

	// Create all the widgets and pack them into the window
	populateWindow();

#if 0
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
#endif

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connect(*this);

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

	if (IsShownOnScreen())
	{
		Hide();
	}

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnect(*this);

	// Destroy the window (after it has been disconnected from the Eventmanager)
	SendDestroyEvent();
	InstancePtr().reset();
}

void SurfaceInspector::connectEvents()
{
#if 0
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
#endif
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
	wxPanel* dialogPanel = new wxPanel(this, wxID_ANY);
	dialogPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* dialogVBox = new wxBoxSizer(wxVERTICAL);

	dialogPanel->GetSizer()->Add(dialogVBox, 1, wxEXPAND | wxALL, 12);

	// Create the title label (bold font)
	wxStaticText* topLabel = new wxStaticText(dialogPanel, wxID_ANY, _(LABEL_PROPERTIES));
	topLabel->SetFont(topLabel->GetFont().Bold());
	
	// 6x2 table with 12 pixel hspacing and 6 pixels vspacing
	wxFlexGridSizer* table = new wxFlexGridSizer(6, 2, 6, 12);
	table->AddGrowableCol(1);

	// Create the entry field and pack it into the first table row
	wxStaticText* shaderLabel = new wxStaticText(dialogPanel, wxID_ANY, _(LABEL_SHADER));
	table->Add(shaderLabel, 0, wxALIGN_CENTRE_VERTICAL);

	wxBoxSizer* shaderHBox = new wxBoxSizer(wxHORIZONTAL);

	_shaderEntry = new wxTextCtrl(dialogPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	_shaderEntry->SetMinSize(wxSize(100, -1));
	_shaderEntry->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(SurfaceInspector::onShaderEntryActivate), NULL, this);
	shaderHBox->Add(_shaderEntry, 1, wxEXPAND);

	// Create the icon button to open the ShaderChooser
	_selectShaderButton = new wxBitmapButton(dialogPanel, wxID_ANY, 
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
	_selectShaderButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onShaderSelect), NULL, this);
	shaderHBox->Add(_selectShaderButton, 0, wxLEFT, 6);

	table->Add(shaderHBox, 1, wxEXPAND);

	// Pack everything into the vbox
	dialogVBox->Add(topLabel, 0, wxEXPAND | wxBOTTOM, 6);
	dialogVBox->Add(table, 0, wxEXPAND | wxLEFT, 18); // 18 pixels left indentation

	// Populate the table with the according widgets
	_manipulators[HSHIFT] = createManipulatorRow(dialogPanel, _(LABEL_HSHIFT), table, false);
	_manipulators[VSHIFT] = createManipulatorRow(dialogPanel, _(LABEL_VSHIFT), table, true);
	_manipulators[HSCALE] = createManipulatorRow(dialogPanel, _(LABEL_HSCALE), table, false);
	_manipulators[VSCALE] = createManipulatorRow(dialogPanel, _(LABEL_VSCALE), table, true);
	_manipulators[ROTATION] = createManipulatorRow(dialogPanel, _(LABEL_ROTATION), table, false);

#if 0
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
#endif
}

SurfaceInspector::ManipulatorRow SurfaceInspector::createManipulatorRow(
	wxWindow* parent, const std::string& label, wxFlexGridSizer* table, bool vertical)
{
	ManipulatorRow manipRow;

	wxStaticText* text = new wxStaticText(parent, wxID_ANY, label);
	table->Add(text, 0, wxALIGN_CENTRE_VERTICAL);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	// Create the entry field
	manipRow.value = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	manipRow.value->SetMinSize(wxSize(60, -1));
	manipRow.value->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(SurfaceInspector::onValueEntryActivate), NULL, this);

	wxBoxSizer* controlButtonBox = NULL;

	if (vertical)
	{
		controlButtonBox = new wxBoxSizer(wxVERTICAL);
		controlButtonBox->SetMinSize(30, 30);

		manipRow.larger = new wxutil::ControlButton(parent, 
			wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "arrow_up.png"));
		manipRow.larger->SetMinSize(wxSize(30, 12));
		controlButtonBox->Add(manipRow.larger, 0);

		manipRow.smaller = new wxutil::ControlButton(parent, 
			wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "arrow_down.png"));
		manipRow.smaller->SetMinSize(wxSize(30, 12));
		controlButtonBox->Add(manipRow.smaller, 0);
	}
	else
	{
		controlButtonBox = new wxBoxSizer(wxHORIZONTAL);
		controlButtonBox->SetMinSize(30, 30);

		manipRow.smaller = new wxutil::ControlButton(parent, 
			wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "arrow_left.png"));
		manipRow.smaller->SetMinSize(wxSize(15, 24));
		controlButtonBox->Add(manipRow.smaller, 0);

		manipRow.larger = new wxutil::ControlButton(parent, 
			wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "arrow_right.png"));
		manipRow.larger->SetMinSize(wxSize(15, 24));
		controlButtonBox->Add(manipRow.larger, 0);
	}

	// Create the label
	wxStaticText* steplabel = new wxStaticText(parent, wxID_ANY, _(LABEL_STEP));

	// Create the entry field
	manipRow.stepEntry = new wxTextCtrl(parent, wxID_ANY, "");
	manipRow.stepEntry->SetMinSize(wxSize(50, -1));

	// Arrange all items in a row
	hbox->Add(manipRow.value, 1);
	hbox->Add(controlButtonBox, 0, wxLEFT, 6);
	hbox->Add(steplabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
	hbox->Add(manipRow.stepEntry, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

	// Pack the hbox into the table
	table->Add(hbox, 1, wxEXPAND);

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
	selection::algorithm::applyShaderToSelection(_shaderEntry->GetValue().ToStdString());

	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
}

void SurfaceInspector::emitTexDef()
{
	TexDef shiftScaleRotate;

	shiftScaleRotate._shift[0] = string::convert<float>(_manipulators[HSHIFT].value->GetValue().ToStdString());
	shiftScaleRotate._shift[1] = string::convert<float>(_manipulators[VSHIFT].value->GetValue().ToStdString());
	shiftScaleRotate._scale[0] = string::convert<float>(_manipulators[HSCALE].value->GetValue().ToStdString());
	shiftScaleRotate._scale[1] = string::convert<float>(_manipulators[VSCALE].value->GetValue().ToStdString());
	shiftScaleRotate._rotate = string::convert<float>(_manipulators[ROTATION].value->GetValue().ToStdString());

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
	_manipulators[HSHIFT].value->SetValue(string::to_string(texdef._shift[0]));
	_manipulators[VSHIFT].value->SetValue(string::to_string(texdef._shift[1]));

	_manipulators[HSCALE].value->SetValue(string::to_string(texdef._scale[0]));
	_manipulators[VSCALE].value->SetValue(string::to_string(texdef._scale[1]));

	_manipulators[ROTATION].value->SetValue(string::to_string(texdef._rotate));
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
#if 0
	const SelectionInfo& selectionInfo = GlobalSelectionSystem().getSelectionInfo();

	bool valueSensitivity = false;
	bool fitSensitivity = (selectionInfo.totalCount > 0);
	bool flipSensitivity = (selectionInfo.totalCount > 0);
	bool applySensitivity = (selectionInfo.totalCount > 0);
	bool alignSensitivity = (selectionInfo.totalCount > 0);

	// If patches or entities are selected, the value entry fields have no meaning
	valueSensitivity = (selectionInfo.patchCount == 0 &&
						selectionInfo.totalCount > 0 &&
						selectionInfo.entityCount == 0 &&
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
#endif
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
		//wxTODO gtkutil::Messagebox::ShowError(_("Both fit values must be > 0."), getRefPtr());
	}
}

void SurfaceInspector::onFit()
{
	// Call the according member method
	fitTexture();
	doUpdate();
}

// The keypress callback for the shift/scale/rotation entry fields
void SurfaceInspector::onValueEntryActivate(wxCommandEvent& ev)
{
	emitTexDef();
	
	// Don't propage the keypress if the Enter could be processed
	this->SetFocus();
}

// The keypress callback
void SurfaceInspector::onShaderEntryActivate(wxCommandEvent& ev)
{
	emitShader();
}

void SurfaceInspector::onShaderSelect(wxCommandEvent& ev)
{
	// Instantiate the modal dialog, will block execution
	/* wxTODO ShaderChooser chooser(this, _shaderEntry);
    chooser.signal_shaderChanged().connect(
        sigc::mem_fun(this, &SurfaceInspector::emitShader)
    );
    chooser.show();*/
}

// Static command target to toggle the window
void SurfaceInspector::toggle(const cmd::ArgumentList& args)
{
	if (!Instance().IsShownOnScreen())
	{
		Instance().Show();
	}
	else
	{
		Instance().Hide();
	}
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
	// Force the focus to the inspector window itself to avoid the cursor
	// from jumping into the shader entry field
	this->SetFocus();
}

void SurfaceInspector::_preHide()
{
	// Save the window position, to make sure
	_windowPosition.readPosition();

	GlobalUndoSystem().removeObserver(this);
	GlobalSelectionSystem().removeObserver(this);
}

} // namespace ui
