#include "SurfaceInspector.h"

#include "i18n.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include "wxutil/Bitmap.h"
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/spinctrl.h>
#include <wx/tglbtn.h>

#include "iscenegraph.h"
#include "ieventmanager.h"
#include "itextstream.h"
#include "imainframe.h"
#include "iundo.h"

#include "wxutil/ControlButton.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/Button.h"

#include "registry/Widgets.h"
#include "selectionlib.h"
#include "math/FloatTools.h"
#include "string/string.h"

#include "textool/TexTool.h"
#include "ui/patch/PatchInspector.h"

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
    const char* const FOLDER_ICON = "treeView16.png";
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

void SurfaceInspector::ManipulatorRow::setValue(double v)
{
    value->SetValue(fmt::format("{}", v));
}

void SurfaceInspector::FitTextureWidgets::enable(bool enabled)
{
    label->Enable(enabled);
    x->Enable(enabled);
    fitButton->Enable(enabled);
    preserveAspectButton->Enable(enabled);
    width->Enable(enabled);
    height->Enable(enabled);
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

void SurfaceInspector::onMainFrameShuttingDown()
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
    _fitTexture.fitButton->Bind(wxEVT_BUTTON,
                                [=](wxCommandEvent&) { onFit(Axis::BOTH); });

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

	wxutil::button::connectToCommand(_flipTexture.flipX, "FlipTextureX");
	wxutil::button::connectToCommand(_flipTexture.flipY, "FlipTextureY");
	wxutil::button::connectToCommand(_modifyTex.natural, "TextureNatural");
	wxutil::button::connectToCommand(_modifyTex.normalise, "NormaliseTexture");

	wxutil::button::connectToCommand(_alignTexture.top, "TexAlignTop");
	wxutil::button::connectToCommand(_alignTexture.bottom, "TexAlignBottom");
	wxutil::button::connectToCommand(_alignTexture.right, "TexAlignRight");
	wxutil::button::connectToCommand(_alignTexture.left, "TexAlignLeft");

	wxutil::button::connectToCommand(_manipulators[HSHIFT].smaller, "TexShiftLeft");
	wxutil::button::connectToCommand(_manipulators[HSHIFT].larger, "TexShiftRight");
	wxutil::button::connectToCommand(_manipulators[VSHIFT].larger, "TexShiftUp");
	wxutil::button::connectToCommand(_manipulators[VSHIFT].smaller, "TexShiftDown");
	wxutil::button::connectToCommand(_manipulators[HSCALE].smaller, "TexScaleLeft");
	wxutil::button::connectToCommand(_manipulators[HSCALE].larger, "TexScaleRight");
	wxutil::button::connectToCommand(_manipulators[VSCALE].larger, "TexScaleUp");
	wxutil::button::connectToCommand(_manipulators[VSCALE].smaller, "TexScaleDown");
	wxutil::button::connectToCommand(_manipulators[ROTATION].larger, "TexRotateClock");
	wxutil::button::connectToCommand(_manipulators[ROTATION].smaller, "TexRotateCounter");
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

wxSpinCtrlDouble* SurfaceInspector::makeFitSpinBox(Axis axis)
{
    wxSpinCtrlDouble* box = new wxSpinCtrlDouble(this, wxID_ANY);

    // Set visual parameters
    box->SetMinSize(wxSize(box->GetCharWidth() * 16, -1));
    box->SetRange(0.0, 1000.0);
    box->SetIncrement(1.0);
    box->SetValue(1.0);
    box->SetDigits(2);

    // Perform a fit operation when the value changes
    box->Bind(wxEVT_SPINCTRLDOUBLE, [=](wxSpinDoubleEvent& e) {
                  if (e.GetValue() > 0)
                      onFit(axis);
              });

    return box;
}

wxBoxSizer* SurfaceInspector::createFitTextureRow()
{
	wxBoxSizer* fitTextureHBox = new wxBoxSizer(wxHORIZONTAL);

    // Create widgets from left to right
	_fitTexture.label = new wxStaticText(this, wxID_ANY, _(LABEL_FIT_TEXTURE));
	_fitTexture.width = makeFitSpinBox(Axis::X);
    _fitTexture.width->SetToolTip(
        _("Number of whole texture images to fit horizontally. Use the spin "
          "buttons to change the value and apply the result immediately.")
    );
    _fitTexture.x = new wxStaticText(this, wxID_ANY, "x");
    _fitTexture.height = makeFitSpinBox(Axis::Y);
    _fitTexture.height->SetToolTip(
        _("Number of whole texture images to fit vertically. Use the spin "
          "buttons to change the value and apply the result immediately.")
    );
	_fitTexture.fitButton = new wxButton(this, wxID_ANY, _(LABEL_FIT));
    _fitTexture.fitButton->SetToolTip(
        _("Fit texture using the current values for both axes (ignoring texture "
          "aspect ratio)")
    );
    _fitTexture.preserveAspectButton = new wxBitmapToggleButton(
        this, wxID_ANY, wxutil::GetLocalBitmap("preserveAspect.png")
    );
    _fitTexture.preserveAspectButton->SetToolTip(
        _("When fitting texture on one axis using the spin buttons, adjust the "
          "other axis automatically to preserve texture aspect ratio")
    );

    // Add widgets to the sizer
    fitTextureHBox->Add(_fitTexture.width, 1, wxALIGN_CENTER_VERTICAL);
    fitTextureHBox->Add(_fitTexture.x, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 3);
    fitTextureHBox->Add(_fitTexture.height, 1, wxALIGN_CENTER_VERTICAL);
    fitTextureHBox->Add(_fitTexture.preserveAspectButton, 0,
                        wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    fitTextureHBox->Add(_fitTexture.fitButton, 1,
                        wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

    return fitTextureHBox;
}

void SurfaceInspector::populateWindow()
{
	wxBoxSizer* dialogVBox = new wxBoxSizer(wxVERTICAL);

	// Create the title label (bold font)
	wxStaticText* topLabel = new wxStaticText(this, wxID_ANY, _(LABEL_PROPERTIES));
	topLabel->SetFont(topLabel->GetFont().Bold());

	// 6x2 table with 12 pixel hspacing and 6 pixels vspacing
	wxFlexGridSizer* table = new wxFlexGridSizer(6, 2, 6, 12);
	table->AddGrowableCol(1);

	// Create the entry field and pack it into the first table row
	wxStaticText* shaderLabel = new wxStaticText(this, wxID_ANY, _(LABEL_SHADER));
	table->Add(shaderLabel, 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* shaderHBox = new wxBoxSizer(wxHORIZONTAL);

	_shaderEntry = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	_shaderEntry->SetMinSize(wxSize(100, -1));
	_shaderEntry->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(SurfaceInspector::onShaderEntryActivate), NULL, this);
	shaderHBox->Add(_shaderEntry, 1, wxEXPAND);

	// Create the icon button to open the ShaderChooser
    _selectShaderButton = new wxBitmapButton(
        this, wxID_ANY, wxutil::GetLocalBitmap(FOLDER_ICON)
    );
    _selectShaderButton->SetToolTip(
        _("Choose shader using the shader selection dialog"))
    ;
    _selectShaderButton->Connect(
        wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onShaderSelect),
        NULL, this
    );
    shaderHBox->Add(_selectShaderButton, 0, wxLEFT, 6);

	table->Add(shaderHBox, 1, wxEXPAND);

	// Pack everything into the vbox
	dialogVBox->Add(topLabel, 0, wxEXPAND | wxBOTTOM, 6);
	dialogVBox->Add(table, 0, wxEXPAND | wxLEFT, 18); // 18 pixels left indentation

	// Populate the table with the according widgets
	_manipulators[HSHIFT] = createManipulatorRow(this, _(LABEL_HSHIFT), table);
	_manipulators[VSHIFT] = createManipulatorRow(this, _(LABEL_VSHIFT), table);
	_manipulators[HSCALE] = createManipulatorRow(this, _(LABEL_HSCALE), table);
	_manipulators[VSCALE] = createManipulatorRow(this, _(LABEL_VSCALE), table);
	_manipulators[ROTATION] = createManipulatorRow(this, _(LABEL_ROTATION), table);

	// ======================== Texture Operations ====================================

	// Create the texture operations label (bold font)
	wxStaticText* operLabel = new wxStaticText(this, wxID_ANY, _(LABEL_OPERATIONS));
	operLabel->SetFont(operLabel->GetFont().Bold());

    // Setup the table with default spacings
	// 5x2 table with 12 pixel hspacing and 6 pixels vspacing
	wxFlexGridSizer* operTable = new wxFlexGridSizer(5, 2, 6, 12);
	operTable->AddGrowableCol(1);

    // Pack label & table into the dialog
	dialogVBox->Add(operLabel, 0, wxEXPAND | wxTOP | wxBOTTOM, 6);
	dialogVBox->Add(operTable, 0, wxEXPAND | wxLEFT, 18); // 18 pixels left indentation

	// ------------------------ Fit Texture -----------------------------------

	wxBoxSizer* fitTextureHBox = createFitTextureRow();
	operTable->Add(_fitTexture.label, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(fitTextureHBox, 1, wxEXPAND);

	// ------------------------ Align Texture -----------------------------------

	_alignTexture.label = new wxStaticText(this, wxID_ANY, _(LABEL_ALIGN_TEXTURE));

	_alignTexture.top = new wxButton(this, wxID_ANY, _(LABEL_ALIGN_TOP));
	_alignTexture.bottom = new wxButton(this, wxID_ANY, _(LABEL_ALIGN_BOTTOM));
	_alignTexture.left = new wxButton(this, wxID_ANY, _(LABEL_ALIGN_LEFT));
	_alignTexture.right = new wxButton(this, wxID_ANY, _(LABEL_ALIGN_RIGHT));

	wxGridSizer* alignTextureBox = new wxGridSizer(1, 4, 0, 6);

	alignTextureBox->Add(_alignTexture.top, 1, wxEXPAND);
	alignTextureBox->Add(_alignTexture.bottom, 1, wxEXPAND);
	alignTextureBox->Add(_alignTexture.left, 1, wxEXPAND);
	alignTextureBox->Add(_alignTexture.right, 1, wxEXPAND);

	operTable->Add(_alignTexture.label, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(alignTextureBox, 1, wxEXPAND);

	// ------------------------ Flip Texture -----------------------------------

	_flipTexture.label = new wxStaticText(this, wxID_ANY, _(LABEL_FLIP_TEXTURE));

	_flipTexture.flipX = new wxButton(this, wxID_ANY, _(LABEL_FLIPX));
	_flipTexture.flipY = new wxButton(this, wxID_ANY, _(LABEL_FLIPY));

	wxGridSizer* flipTextureBox = new wxGridSizer(1, 2, 0, 6);

	flipTextureBox->Add(_flipTexture.flipX, 1, wxEXPAND);
	flipTextureBox->Add(_flipTexture.flipY, 1, wxEXPAND);

	operTable->Add(_flipTexture.label, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(flipTextureBox, 1, wxEXPAND);

	// ------------------------ Modify Texture -----------------------------------

	_modifyTex.label = new wxStaticText(this, wxID_ANY, _(LABEL_MODIFY_TEXTURE));

	_modifyTex.natural = new wxButton(this, wxID_ANY, _(LABEL_NATURAL));
	_modifyTex.normalise = new wxButton(this, wxID_ANY, _(LABEL_NORMALISE));

	wxGridSizer* modTextureBox = new wxGridSizer(1, 2, 0, 6);

	modTextureBox->Add(_modifyTex.natural, 1, wxEXPAND);
	modTextureBox->Add(_modifyTex.normalise, 1, wxEXPAND);

	operTable->Add(_modifyTex.label, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(modTextureBox, 1, wxEXPAND);

	// ------------------------ Default Scale -----------------------------------

	wxStaticText* defaultScaleLabel = new wxStaticText(this, wxID_ANY, _(LABEL_DEFAULT_SCALE));

	_defaultTexScale = new wxSpinCtrlDouble(this, wxID_ANY);
	_defaultTexScale->SetMinSize(wxSize(55, -1));
	_defaultTexScale->SetRange(0.0, 1000.0);
	_defaultTexScale->SetIncrement(0.1);
	_defaultTexScale->SetDigits(3);

	// Texture Lock Toggle
	_texLockButton = new wxToggleButton(this, wxID_ANY, _(LABEL_TEXTURE_LOCK));

	wxGridSizer* defaultScaleBox = new wxGridSizer(1, 2, 0, 6);

    wxBoxSizer* texScaleSizer = new wxBoxSizer(wxHORIZONTAL);
    texScaleSizer->Add(_defaultTexScale, 1, wxALIGN_CENTER_VERTICAL);

    defaultScaleBox->Add(texScaleSizer, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    defaultScaleBox->Add(_texLockButton, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

	operTable->Add(defaultScaleLabel, 0, wxALIGN_CENTER_VERTICAL);
	operTable->Add(defaultScaleBox, 1, wxEXPAND);

    wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
    border->Add(dialogVBox, 1, wxEXPAND | wxALL, 12);
	SetSizerAndFit(border);
}

SurfaceInspector::ManipulatorRow SurfaceInspector::createManipulatorRow(
	wxWindow* parent, const std::string& label, wxFlexGridSizer* table)
{
	ManipulatorRow manipRow;

	wxStaticText* text = new wxStaticText(parent, wxID_ANY, label);
	table->Add(text, 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	// Create the entry field
	manipRow.value = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	manipRow.value->SetMinSize(wxSize(60, -1));
	manipRow.value->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(SurfaceInspector::onValueEntryActivate), NULL, this);

    // Create the nudge buttons
	wxBoxSizer* controlButtonBox = new wxBoxSizer(wxHORIZONTAL);
    manipRow.smaller = new wxutil::ControlButton(parent,
        wxutil::GetLocalBitmap("arrow_left.png"));
    controlButtonBox->Add(manipRow.smaller, 0);
    controlButtonBox->AddSpacer(2);
    manipRow.larger = new wxutil::ControlButton(parent,
        wxutil::GetLocalBitmap("arrow_right.png"));
    controlButtonBox->Add(manipRow.larger, 0);

	// Create the label
	wxStaticText* steplabel = new wxStaticText(parent, wxID_ANY, _(LABEL_STEP));

	// Create the entry field
	manipRow.stepEntry = new wxTextCtrl(parent, wxID_ANY, "");

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

		// Pre-destruction cleanup
		GlobalMainFrame().signal_MainFrameShuttingDown().connect(
            sigc::mem_fun(*instancePtr, &SurfaceInspector::onMainFrameShuttingDown)
        );
	}

	return *instancePtr;
}

void SurfaceInspector::emitShader()
{
	// Apply it to the selection
    selection::applyShaderToSelection(_shaderEntry->GetValue().ToStdString());
}

void SurfaceInspector::emitTexDef()
{
	ShiftScaleRotation shiftScaleRotate;

	shiftScaleRotate.shift[0] = string::convert<float>(_manipulators[HSHIFT].value->GetValue().ToStdString());
	shiftScaleRotate.shift[1] = string::convert<float>(_manipulators[VSHIFT].value->GetValue().ToStdString());
	shiftScaleRotate.scale[0] = string::convert<float>(_manipulators[HSCALE].value->GetValue().ToStdString());
	shiftScaleRotate.scale[1] = string::convert<float>(_manipulators[VSCALE].value->GetValue().ToStdString());
	shiftScaleRotate.rotate = string::convert<float>(_manipulators[ROTATION].value->GetValue().ToStdString());

	// Apply it to the selection
	UndoableCommand undo("textureDefinitionSetSelected");

	GlobalSelectionSystem().foreachFace([&](IFace& face)
	{
		face.setShiftScaleRotation(shiftScaleRotate);
	});

	SceneChangeNotify();

	// Update the Texture Tools
	radiant::TextureChangedMessage::Send();
	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
}

void SurfaceInspector::updateTexDef()
{
	if (GlobalSelectionSystem().getSelectedFaceCount() == 1)
	{
        // This call should return a meaningful value, since we only get here when only
        // a single face is selected
        auto& face = GlobalSelectionSystem().getSingleSelectedFace();

		auto texdef = face.getShiftScaleRotation();

	    // Snap the floating point variables to the max resolution to avoid things like "1.45e-14"
	    texdef.shift[0] = float_snapped(texdef.shift[0], MAX_FLOAT_RESOLUTION);
	    texdef.shift[1] = float_snapped(texdef.shift[1], MAX_FLOAT_RESOLUTION);
	    texdef.scale[0] = float_snapped(texdef.scale[0], MAX_FLOAT_RESOLUTION);
	    texdef.scale[1] = float_snapped(texdef.scale[1], MAX_FLOAT_RESOLUTION);
	    texdef.rotate = float_snapped(texdef.rotate, MAX_FLOAT_RESOLUTION);

	    // Load the values into the widgets
	    _manipulators[HSHIFT].setValue(texdef.shift[0]);
	    _manipulators[VSHIFT].setValue(texdef.shift[1]);
	    _manipulators[HSCALE].setValue(texdef.scale[0]);
	    _manipulators[VSCALE].setValue(texdef.scale[1]);
	    _manipulators[ROTATION].setValue(texdef.rotate);
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
	bool haveSelection = (selectionInfo.totalCount > 0);

	// If patches or entities are selected, the value entry fields have no meaning
	valueSensitivity = (selectionInfo.patchCount == 0 &&
						selectionInfo.totalCount > 0 &&
						selectionInfo.entityCount == 0 &&
						GlobalSelectionSystem().getSelectedFaceCount() == 1);

	_manipulators[HSHIFT].value->Enable(valueSensitivity);
	_manipulators[VSHIFT].value->Enable(valueSensitivity);
	_manipulators[HSCALE].value->Enable(valueSensitivity);
	_manipulators[VSCALE].value->Enable(valueSensitivity);
	_manipulators[ROTATION].value->Enable(valueSensitivity);

	// The fit widget sensitivity
    _fitTexture.enable(haveSelection);

	// The align texture widget sensitivity
	_alignTexture.bottom->Enable(haveSelection);
	_alignTexture.left->Enable(haveSelection);
	_alignTexture.right->Enable(haveSelection);
	_alignTexture.top->Enable(haveSelection);
	_alignTexture.label->Enable(haveSelection);

	// The flip texture widget sensitivity
	_flipTexture.label->Enable(haveSelection);
	_flipTexture.flipX->Enable(haveSelection);
	_flipTexture.flipY->Enable(haveSelection);

	// The natural/normalise widget sensitivity
	_modifyTex.label->Enable(haveSelection);
	_modifyTex.natural->Enable(haveSelection);
	_modifyTex.normalise->Enable(haveSelection);

	// Current shader name
	_shaderEntry->SetValue(selection::getShaderFromSelection());

	if (valueSensitivity)
	{
		updateTexDef();
	}

	// Update the TexTool instance as well
	ui::TexTool::Instance().draw();
	ui::PatchInspector::Instance().queueUpdate();
}

void SurfaceInspector::fitTexture(Axis axis)
{
    // If the preserve aspect button is disabled, we always fit on both axes
    if (!_fitTexture.preserveAspectButton->GetValue())
        axis = Axis::BOTH;

	double repeatX = _fitTexture.width->GetValue();
	double repeatY = _fitTexture.height->GetValue();

	if (repeatX > 0.0 && repeatY > 0.0)
	{
        GlobalCommandSystem().executeCommand("FitTexture", repeatX, repeatY);
    }
	else
	{
		// Invalid repeatX && repeatY values
		wxutil::Messagebox::ShowError(_("Both fit values must be > 0."));
        return;
	}

    // If only fitting on a single axis, propagate the scale factor for the
    // fitted axis to the non-fitted axis, which should preserve the texture
    // aspect ratio.
    if (axis != Axis::BOTH)
    {
        GlobalSelectionSystem().foreachFace(
            [&](IFace& face) {
                ShiftScaleRotation texdef = face.getShiftScaleRotation();
                if (axis == Axis::X)
                    texdef.scale[1] = texdef.scale[0];
                else
                    texdef.scale[0] = texdef.scale[1];
                face.setShiftScaleRotation(texdef);
            }
        );
    }
}

void SurfaceInspector::onFit(Axis axis)
{
    // Fit the texture then update our widget values with the new transform
	fitTexture(axis);
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
    // Remember this shader, we need it for the final apply call
    std::string previousShader = _shaderEntry->GetValue().ToStdString();

	// Instantiate the modal dialog, will block execution
	auto* chooser = new ShaderChooser(this, _shaderEntry);

    chooser->signal_shaderChanged().connect(
        sigc::mem_fun(this, &SurfaceInspector::emitShader)
    );

    if (chooser->ShowModal() == wxID_OK && _shaderEntry->GetValue().ToStdString() != previousShader)
    {
        // Revert the shader first, this is the starting point for the transaction
        selection::applyShaderToSelection(previousShader);

        // Perform the final apply, opening an UndoableCommand first
        UndoableCommand cmd("ApplyShaderToSelection " + _shaderEntry->GetValue().ToStdString());
        selection::applyShaderToSelection(_shaderEntry->GetValue().ToStdString());
    }

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

	GlobalRadiantCore().getMessageBus().removeListener(_textureMessageHandler);
	_textureMessageHandler = 0;

	_undoHandler.disconnect();
	_redoHandler.disconnect();

	// Register self to the SelSystem to get notified upon selection changes.
	_selectionChanged = GlobalSelectionSystem().signal_selectionChanged().connect(
		[this] (const ISelectable&) { doUpdate(); });

	_undoHandler = GlobalUndoSystem().signal_postUndo().connect(
		sigc::mem_fun(this, &SurfaceInspector::doUpdate));
	_redoHandler = GlobalUndoSystem().signal_postRedo().connect(
		sigc::mem_fun(this, &SurfaceInspector::doUpdate));

	// Get notified about texture changes
	_textureMessageHandler = GlobalRadiantCore().getMessageBus().addListener(
		radiant::IMessage::Type::TextureChanged,
		radiant::TypeListener<radiant::TextureChangedMessage>(
			sigc::mem_fun(this, &SurfaceInspector::handleTextureChangedMessage)));

	// Re-scan the selection
	doUpdate();
}

void SurfaceInspector::handleTextureChangedMessage(radiant::TextureChangedMessage& msg)
{
	_updateNeeded = true;
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

	GlobalRadiantCore().getMessageBus().removeListener(_textureMessageHandler);
	_textureMessageHandler = 0;

	_selectionChanged.disconnect();
	_undoHandler.disconnect();
	_redoHandler.disconnect();
}

} // namespace ui
