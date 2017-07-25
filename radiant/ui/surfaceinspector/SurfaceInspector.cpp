#include "SurfaceInspector.h"

#include "i18n.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/artprov.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/spinctrl.h>
#include <wx/tglbtn.h>

#include "ieventmanager.h"
#include "itextstream.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "iundo.h"

#include "wxutil/ControlButton.h"
#include "wxutil/dialog/MessageBox.h"

#include "registry/Widgets.h"
#include "selectionlib.h"
#include "math/FloatTools.h"
#include "string/string.h"

#include "textool/TexTool.h"
#include "ui/patch/PatchInspector.h"
#include "brush/TextureProjection.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Shader.h"
#include "brush/Face.h"
#include "brush/Brush.h"
#include "patch/Patch.h"

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

    const char* LABEL_MODIFY_TEXTURE = N_("Modify Texture:");
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
	_callbackActive(false),
	_updateNeeded(false)
{
	Connect(wxEVT_IDLE, wxIdleEventHandler(SurfaceInspector::onIdle), NULL, this);

	// Create all the widgets and pack them into the window
	populateWindow();

	// Connect the defaultTexScale widget to its registry key
	registry::bindWidget(_defaultTexScale, RKEY_DEFAULT_TEXTURE_SCALE);

	// Connect the step values to the according registry values
    registry::bindWidget(_manipulators[HSHIFT].stepEntry, RKEY_HSHIFT_STEP);
    registry::bindWidget(_manipulators[VSHIFT].stepEntry, RKEY_VSHIFT_STEP);
    registry::bindWidget(_manipulators[HSCALE].stepEntry, RKEY_HSCALE_STEP);
    registry::bindWidget(_manipulators[VSCALE].stepEntry, RKEY_VSCALE_STEP);
    registry::bindWidget(_manipulators[ROTATION].stepEntry, RKEY_ROTATION_STEP);

	// Be notified upon key changes
	GlobalRegistry().signalForKey(RKEY_DEFAULT_TEXTURE_SCALE).connect(
        sigc::mem_fun(this, &SurfaceInspector::keyChanged)
    );

	// Update the widget status
	doUpdate();

	// Get the relevant Events from the Manager and connect the widgets
	connectEvents();

	InitialiseWindowPosition(410, 480, RKEY_WINDOW_STATE);
}

SurfaceInspectorPtr& SurfaceInspector::InstancePtr()
{
	static SurfaceInspectorPtr _instancePtr;
	return _instancePtr;
}

void SurfaceInspector::onRadiantShutdown()
{
	rMessage() << "SurfaceInspector shutting down." << std::endl;

	if (IsShownOnScreen())
	{
		Hide();
	}

	// Destroy the window
	SendDestroyEvent();
	InstancePtr().reset();
}

void SurfaceInspector::connectEvents()
{
	// Be sure to connect these signals BEFORE the buttons are connected
	// to the events, so that the doUpdate() call gets invoked after the actual event has been fired.
	_fitTexture.button->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onFit), NULL, this);

	_flipTexture.flipX->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
	_flipTexture.flipY->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
	_alignTexture.top->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
	_alignTexture.bottom->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
	_alignTexture.right->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
	_alignTexture.left->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
	_modifyTex.natural->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
	_modifyTex.normalise->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);

	for (ManipulatorMap::iterator i = _manipulators.begin(); i != _manipulators.end(); ++i)
	{
		i->second.smaller->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
		i->second.larger->Connect(wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onUpdateAfterButtonClick), NULL, this);
	}
	
	// Connect the ToggleTexLock item to the according command
	GlobalEventManager().findEvent("TogTexLock")->connectToggleButton(_texLockButton);

	GlobalEventManager().findEvent("FlipTextureX")->connectButton(_flipTexture.flipX);
	GlobalEventManager().findEvent("FlipTextureY")->connectButton(_flipTexture.flipY);
	GlobalEventManager().findEvent("TextureNatural")->connectButton(_modifyTex.natural);
	GlobalEventManager().findEvent("NormaliseTexture")->connectButton(_modifyTex.normalise);

	GlobalEventManager().findEvent("TexAlignTop")->connectButton(_alignTexture.top);
	GlobalEventManager().findEvent("TexAlignBottom")->connectButton(_alignTexture.bottom);
	GlobalEventManager().findEvent("TexAlignRight")->connectButton(_alignTexture.right);
	GlobalEventManager().findEvent("TexAlignLeft")->connectButton(_alignTexture.left);

	GlobalEventManager().findEvent("TexShiftLeft")->connectButton(_manipulators[HSHIFT].smaller);
	GlobalEventManager().findEvent("TexShiftRight")->connectButton(_manipulators[HSHIFT].larger);
	GlobalEventManager().findEvent("TexShiftUp")->connectButton(_manipulators[VSHIFT].larger);
	GlobalEventManager().findEvent("TexShiftDown")->connectButton(_manipulators[VSHIFT].smaller);
	GlobalEventManager().findEvent("TexScaleLeft")->connectButton(_manipulators[HSCALE].smaller);
	GlobalEventManager().findEvent("TexScaleRight")->connectButton(_manipulators[HSCALE].larger);
	GlobalEventManager().findEvent("TexScaleUp")->connectButton(_manipulators[VSCALE].larger);
	GlobalEventManager().findEvent("TexScaleDown")->connectButton(_manipulators[VSCALE].smaller);
	GlobalEventManager().findEvent("TexRotateClock")->connectButton(_manipulators[ROTATION].larger);
	GlobalEventManager().findEvent("TexRotateCounter")->connectButton(_manipulators[ROTATION].smaller);
}

void SurfaceInspector::keyChanged()
{
	// Avoid callback loops
	if (_callbackActive) {
		return;
	}

	_callbackActive = true;

	_defaultTexScale->SetValue(registry::getValue<double>(RKEY_DEFAULT_TEXTURE_SCALE));

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
	table->Add(shaderLabel, 0, wxALIGN_CENTER_VERTICAL);

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

	// ======================== Texture Operations ====================================

	// Create the texture operations label (bold font)
	wxStaticText* operLabel = new wxStaticText(dialogPanel, wxID_ANY, _(LABEL_OPERATIONS));
	operLabel->SetFont(operLabel->GetFont().Bold());

    // Setup the table with default spacings
	// 5x2 table with 12 pixel hspacing and 6 pixels vspacing
	wxFlexGridSizer* operTable = new wxFlexGridSizer(5, 2, 6, 12);
	operTable->AddGrowableCol(1);

    // Pack label & table into the dialog
	dialogVBox->Add(operLabel, 0, wxEXPAND | wxTOP | wxBOTTOM, 6);
	dialogVBox->Add(operTable, 0, wxEXPAND | wxLEFT, 18); // 18 pixels left indentation

	// ------------------------ Fit Texture -----------------------------------

	wxBoxSizer* fitTextureHBox = new wxBoxSizer(wxHORIZONTAL);

	// Create the "Fit Texture" label
	_fitTexture.label = new wxStaticText(dialogPanel, wxID_ANY, _(LABEL_FIT_TEXTURE));
	
	// Create the width entry field
	_fitTexture.width = new wxSpinCtrlDouble(dialogPanel, wxID_ANY);
    _fitTexture.width->SetMinSize(wxSize(_fitTexture.width->GetCharWidth() * 8, -1));
	_fitTexture.width->SetRange(0.0, 1000.0);
	_fitTexture.width->SetIncrement(1.0);
	_fitTexture.width->SetValue(1.0);
    _fitTexture.width->SetDigits(2);

	// Create the "x" label
	_fitTexture.x = new wxStaticText(dialogPanel, wxID_ANY, "x");

	// Create the height entry field
	_fitTexture.height = new wxSpinCtrlDouble(dialogPanel, wxID_ANY);
    _fitTexture.height->SetMinSize(wxSize(_fitTexture.height->GetCharWidth() * 8, -1));
	_fitTexture.height->SetRange(0.0, 1000.0);
	_fitTexture.height->SetIncrement(1.0);
	_fitTexture.height->SetValue(1.0);
    _fitTexture.height->SetDigits(2);

	_fitTexture.button = new wxButton(dialogPanel, wxID_ANY, _(LABEL_FIT));

	fitTextureHBox->Add(_fitTexture.width, 0, wxALIGN_CENTER_VERTICAL);
	fitTextureHBox->Add(_fitTexture.x, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 3);
    fitTextureHBox->Add(_fitTexture.height, 0, wxALIGN_CENTER_VERTICAL);
    fitTextureHBox->Add(_fitTexture.button, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

	operTable->Add(_fitTexture.label, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(fitTextureHBox, 1, wxEXPAND);

	// ------------------------ Align Texture -----------------------------------

	_alignTexture.label = new wxStaticText(dialogPanel, wxID_ANY, _(LABEL_ALIGN_TEXTURE));

	_alignTexture.top = new wxButton(dialogPanel, wxID_ANY, _(LABEL_ALIGN_TOP));
	_alignTexture.bottom = new wxButton(dialogPanel, wxID_ANY, _(LABEL_ALIGN_BOTTOM));
	_alignTexture.left = new wxButton(dialogPanel, wxID_ANY, _(LABEL_ALIGN_LEFT));
	_alignTexture.right = new wxButton(dialogPanel, wxID_ANY, _(LABEL_ALIGN_RIGHT));

	wxGridSizer* alignTextureBox = new wxGridSizer(1, 4, 0, 6);

	alignTextureBox->Add(_alignTexture.top, 1, wxEXPAND);
	alignTextureBox->Add(_alignTexture.bottom, 1, wxEXPAND);
	alignTextureBox->Add(_alignTexture.left, 1, wxEXPAND);
	alignTextureBox->Add(_alignTexture.right, 1, wxEXPAND);

	operTable->Add(_alignTexture.label, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(alignTextureBox, 1, wxEXPAND);

	// ------------------------ Flip Texture -----------------------------------

	_flipTexture.label = new wxStaticText(dialogPanel, wxID_ANY, _(LABEL_FLIP_TEXTURE));

	_flipTexture.flipX = new wxButton(dialogPanel, wxID_ANY, _(LABEL_FLIPX));
	_flipTexture.flipY = new wxButton(dialogPanel, wxID_ANY, _(LABEL_FLIPY));

	wxGridSizer* flipTextureBox = new wxGridSizer(1, 2, 0, 6);

	flipTextureBox->Add(_flipTexture.flipX, 1, wxEXPAND);
	flipTextureBox->Add(_flipTexture.flipY, 1, wxEXPAND);

	operTable->Add(_flipTexture.label, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(flipTextureBox, 1, wxEXPAND);

	// ------------------------ Modify Texture -----------------------------------

	_modifyTex.label = new wxStaticText(dialogPanel, wxID_ANY, _(LABEL_MODIFY_TEXTURE));

	_modifyTex.natural = new wxButton(dialogPanel, wxID_ANY, _(LABEL_NATURAL));
	_modifyTex.normalise = new wxButton(dialogPanel, wxID_ANY, _(LABEL_NORMALISE));

	wxGridSizer* modTextureBox = new wxGridSizer(1, 2, 0, 6);

	modTextureBox->Add(_modifyTex.natural, 1, wxEXPAND);
	modTextureBox->Add(_modifyTex.normalise, 1, wxEXPAND);

	operTable->Add(_modifyTex.label, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(modTextureBox, 1, wxEXPAND);

	// ------------------------ Default Scale -----------------------------------

	wxStaticText* defaultScaleLabel = new wxStaticText(dialogPanel, wxID_ANY, _(LABEL_DEFAULT_SCALE));

	_defaultTexScale = new wxSpinCtrlDouble(dialogPanel, wxID_ANY);
	_defaultTexScale->SetMinSize(wxSize(55, -1));
	_defaultTexScale->SetRange(0.0, 1000.0);
	_defaultTexScale->SetIncrement(0.1);
	_defaultTexScale->SetDigits(3);

	// Texture Lock Toggle
	_texLockButton = new wxToggleButton(dialogPanel, wxID_ANY, _(LABEL_TEXTURE_LOCK));

	wxGridSizer* defaultScaleBox = new wxGridSizer(1, 2, 0, 6);

    wxBoxSizer* texScaleSizer = new wxBoxSizer(wxHORIZONTAL);
    texScaleSizer->Add(_defaultTexScale, 1, wxALIGN_CENTER_VERTICAL);

    defaultScaleBox->Add(texScaleSizer, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    defaultScaleBox->Add(_texLockButton, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

	operTable->Add(defaultScaleLabel, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(defaultScaleBox, 1, wxEXPAND);
}

SurfaceInspector::ManipulatorRow SurfaceInspector::createManipulatorRow(
	wxWindow* parent, const std::string& label, wxFlexGridSizer* table, bool vertical)
{
	ManipulatorRow manipRow;

	wxStaticText* text = new wxStaticText(parent, wxID_ANY, label);
	table->Add(text, 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	// Create the entry field
	manipRow.value = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	manipRow.value->SetMinSize(wxSize(60, -1));
	manipRow.value->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(SurfaceInspector::onValueEntryActivate), NULL, this);

	wxBoxSizer* controlButtonBox = NULL;

	if (vertical)
	{
		controlButtonBox = new wxBoxSizer(wxVERTICAL);
        controlButtonBox->SetMinSize(30, 24);

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
        controlButtonBox->SetMinSize(24, 24);

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
	manipRow.stepEntry->SetMinSize(wxSize(40, -1));

	// Arrange all items in a row
    hbox->Add(manipRow.value, 1, wxALIGN_CENTER_VERTICAL);
    hbox->Add(controlButtonBox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
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

	// Apply it to the selection
	selection::algorithm::applyTexDefToFaces(shiftScaleRotate);

	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
}

void SurfaceInspector::updateTexDef()
{
    try
    {
        Face& face = selection::algorithm::getLastSelectedFace();

        // This call should return a meaningful value, since we only get here when only
        // a single face is selected
        TextureProjection curProjection;
        face.GetTexdef(curProjection);

        // Multiply the texture dimensions to the projection matrix such that 
        // the shift/scale/rotation represent pixel values within the image.
        Vector2 shaderDims(face.getFaceShader().getWidth(), face.getFaceShader().getHeight());

        TextureMatrix bpTexDef= curProjection.matrix;
        bpTexDef.applyShaderDimensions(static_cast<std::size_t>(shaderDims[0]), static_cast<std::size_t>(shaderDims[1]));

	    // Calculate the "fake" texture properties (shift/scale/rotation)
	    TexDef texdef = bpTexDef.getFakeTexCoords();

	    if (shaderDims != Vector2(0,0))
        {
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
    catch (selection::InvalidSelectionException&)
    {
        rError() << "Can't update texdef, since more than one face is selected." << std::endl;
    }
}

// Public soft update function
void SurfaceInspector::update()
{
    if (InstancePtr())
    {
		Instance()._updateNeeded = true;
    }
}

void SurfaceInspector::onIdle(wxIdleEvent& ev)
{
	if (_updateNeeded)
	{
		doUpdate();
	}
}

void SurfaceInspector::doUpdate()
{
	_updateNeeded = false;

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

	_manipulators[HSHIFT].value->Enable(valueSensitivity);
	_manipulators[VSHIFT].value->Enable(valueSensitivity);
	_manipulators[HSCALE].value->Enable(valueSensitivity);
	_manipulators[VSCALE].value->Enable(valueSensitivity);
	_manipulators[ROTATION].value->Enable(valueSensitivity);

	// The fit widget sensitivity
	_fitTexture.height->Enable(fitSensitivity);
	_fitTexture.width->Enable(fitSensitivity);
	_fitTexture.x->Enable(fitSensitivity);
	_fitTexture.label->Enable(fitSensitivity);
	_fitTexture.button->Enable(fitSensitivity);

	// The align texture widget sensitivity
	_alignTexture.bottom->Enable(alignSensitivity);
	_alignTexture.left->Enable(alignSensitivity);
	_alignTexture.right->Enable(alignSensitivity);
	_alignTexture.top->Enable(alignSensitivity);
	_alignTexture.label->Enable(alignSensitivity);

	// The flip texture widget sensitivity
	_flipTexture.label->Enable(flipSensitivity);
	_flipTexture.flipX->Enable(flipSensitivity);
	_flipTexture.flipY->Enable(flipSensitivity);

	// The natural/normalise widget sensitivity
	_modifyTex.label->Enable(applySensitivity);
	_modifyTex.natural->Enable(applySensitivity);
	_modifyTex.normalise->Enable(applySensitivity);

	// Current shader name
	_shaderEntry->SetValue(selection::algorithm::getShaderFromSelection());

	if (valueSensitivity)
	{
		updateTexDef();
	}

	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
	ui::PatchInspector::Instance().queueUpdate();
}

void SurfaceInspector::fitTexture()
{
	double repeatX = _fitTexture.width->GetValue();
	double repeatY = _fitTexture.height->GetValue();

	if (repeatX > 0.0 && repeatY > 0.0)
	{
		selection::algorithm::fitTexture(repeatX, repeatY);
	}
	else
	{
		// Invalid repeatX && repeatY values
		wxutil::Messagebox::ShowError(_("Both fit values must be > 0."));
	}
}

void SurfaceInspector::onFit(wxCommandEvent& ev)
{
	// Call the according member method
	fitTexture();
	doUpdate();
}

void SurfaceInspector::onUpdateAfterButtonClick(wxCommandEvent& ev)
{
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
	ShaderChooser* chooser = new ShaderChooser(this, _shaderEntry);

    chooser->signal_shaderChanged().connect(
        sigc::mem_fun(this, &SurfaceInspector::emitShader)
    );

    chooser->ShowModal();
	chooser->Destroy();
}

// Static command target to toggle the window
void SurfaceInspector::toggle(const cmd::ArgumentList& args)
{
	Instance().ToggleVisibility();
}

// TransientWindow callbacks
void SurfaceInspector::_preShow()
{
	TransientWindow::_preShow();

	// Disconnect everything, in some cases we get two consecutive Show() calls in wxGTK
	_selectionChanged.disconnect();
	_brushFaceShaderChanged.disconnect();
	_faceTexDefChanged.disconnect();
	_patchTextureChanged.disconnect();
	_undoHandler.disconnect();
	_redoHandler.disconnect();

	// Register self to the SelSystem to get notified upon selection changes.
	_selectionChanged = GlobalSelectionSystem().signal_selectionChanged().connect(
		[this] (const ISelectable&) { doUpdate(); });

	_undoHandler = GlobalUndoSystem().signal_postUndo().connect(
		sigc::mem_fun(this, &SurfaceInspector::doUpdate));
	_redoHandler = GlobalUndoSystem().signal_postRedo().connect(
		sigc::mem_fun(this, &SurfaceInspector::doUpdate));

	// Get notified about face shader changes
	_brushFaceShaderChanged = Brush::signal_faceShaderChanged().connect(
		[this] { _updateNeeded = true; });

	_faceTexDefChanged = Face::signal_texdefChanged().connect(
		[this] { _updateNeeded = true; });

	_patchTextureChanged = Patch::signal_patchTextureChanged().connect(
		[this] { _updateNeeded = true; });

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
	TransientWindow::_preHide();

	_selectionChanged.disconnect();
	_patchTextureChanged.disconnect();
	_faceTexDefChanged.disconnect();
	_brushFaceShaderChanged.disconnect();
	_undoHandler.disconnect();
	_redoHandler.disconnect();
}

} // namespace ui
