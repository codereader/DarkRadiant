#include "SurfaceInspector.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "ui/ieventmanager.h"
#include "iundo.h"

#include <wx/wx.h>
#include <wx/statline.h>

#include "wxutil/Bitmap.h"
#include "wxutil/ControlButton.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/Button.h"
#include "wxutil/BitmapToggleButton.h"

#include "registry/Widgets.h"
#include "selectionlib.h"
#include "math/FloatTools.h"
#include "string/convert.h"

#include "ui/materials/MaterialChooser.h"

namespace ui
{

namespace
{
    constexpr const char* const LABEL_PROPERTIES = N_("Texture Properties");
    constexpr const char* const LABEL_OPERATIONS = N_("Texture Operations");

    const std::string HSHIFT = "horizshift";
    const std::string VSHIFT = "vertshift";
    const std::string HSCALE = "horizscale";
    const std::string VSCALE = "vertscale";
    const std::string ROTATION = "rotation";

    constexpr const char* const LABEL_HSHIFT = N_("Horiz. Shift:");
    constexpr const char* const LABEL_VSHIFT = N_("Vert. Shift:");
    constexpr const char* const LABEL_HSCALE = N_("Horiz. Scale:");
    constexpr const char* const LABEL_VSCALE = N_("Vert. Scale:");
    constexpr const char* const LABEL_ROTATION = N_("Rotation:");
    constexpr const char* const LABEL_SHADER = N_("Shader:");
    constexpr const char* const FOLDER_ICON = "treeView16.png";
    constexpr const char* const LABEL_STEP = N_("Step:");

    constexpr const char* LABEL_FIT_TEXTURE = N_("Fit:");
    constexpr const char* LABEL_FIT = N_("Fit");

    constexpr const char* LABEL_ALIGN_TEXTURE = N_("Align:");
    constexpr const char* LABEL_ALIGN_TOP = N_("Top");
    constexpr const char* LABEL_ALIGN_BOTTOM = N_("Bottom");
    constexpr const char* LABEL_ALIGN_RIGHT = N_("Right");
    constexpr const char* LABEL_ALIGN_LEFT = N_("Left");

    constexpr const char* LABEL_FLIP_TEXTURE = N_("Flip:");
    constexpr const char* LABEL_FLIPX = N_("Flip Horizontal");
    constexpr const char* LABEL_FLIPY = N_("Flip Vertical");

    constexpr const char* LABEL_MODIFY_TEXTURE = N_("Modify:");
    constexpr const char* LABEL_NATURAL = N_("Natural");
    constexpr const char* LABEL_NORMALISE = N_("Normalise");

    constexpr const char* LABEL_DEFAULT_SCALE = N_("Default Scale:");
    constexpr const char* LABEL_TEXTURE_LOCK = N_("Texture Lock");

    const std::string RKEY_DEFAULT_TEXTURE_SCALE = "user/ui/textures/defaultTextureScale";

    const std::string RKEY_ROOT = "user/ui/textures/surfaceInspector/";
    const std::string RKEY_HSHIFT_STEP = RKEY_ROOT + "hShiftStep";
    const std::string RKEY_VSHIFT_STEP = RKEY_ROOT + "vShiftStep";
    const std::string RKEY_HSCALE_STEP = RKEY_ROOT + "hScaleStep";
    const std::string RKEY_VSCALE_STEP = RKEY_ROOT + "vScaleStep";
    const std::string RKEY_ROTATION_STEP = RKEY_ROOT + "rotStep";

    constexpr double MAX_FLOAT_RESOLUTION = 1.0E-5;

    // Widget minimum sizes. Different values needed on Linux (which has various
    // GTK styles) vs Windows.
#ifdef __WXMSW__
    constexpr int SPINBOX_WIDTH_CHARS = 7;
#else
    constexpr int SPINBOX_WIDTH_CHARS = 16;
#endif

    // Minimum pixel size for a widget. Converts to a wxSize of the specified
    // dimensions on Windows, and wxDefaultSize on Linux (because pixel sizes
    // don't work with GTK themes and variable DPI)
    struct PixelSize: public wxSize
    {
#if defined(__WXMSW__)
        PixelSize(int width, int height)
        : wxSize(width, height)
        {}
#else
        PixelSize(int width, int height)
        : wxSize(wxDefaultSize.GetX(), wxDefaultSize.GetY())
        {}
#endif
    };
}

void SurfaceInspector::ManipulatorRow::setValue(double v)
{
    value->SetValue(fmt::format("{0:g}", v));
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

SurfaceInspector::SurfaceInspector(wxWindow* parent) :
    DockablePanel(parent),
	_callbackActive(false),
	_updateNeeded(false)
{
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

	// Get the relevant Events from the Manager and connect the widgets
	connectButtons();

    // Force the focus to the inspector window itself to avoid the cursor
    // from jumping into the shader entry field
    this->SetFocus();
}

SurfaceInspector::~SurfaceInspector()
{
    if (panelIsActive())
    {
        disconnectEventHandlers();
    }
}

void SurfaceInspector::onPanelActivated()
{
    connectEventHandlers();

    // Re-scan the selection
    doUpdate();
}

void SurfaceInspector::onPanelDeactivated()
{
    disconnectEventHandlers();
}

void SurfaceInspector::connectEventHandlers()
{
    // Connect the ToggleTexLock item to the corresponding command
    GlobalEventManager().findEvent("TogTexLock")->connectToggleButton(_texLockButton);

    // Register self to the SelSystem to get notified upon selection changes.
    _selectionChanged = GlobalSelectionSystem().signal_selectionChanged().connect(
        [this](const ISelectable&) { doUpdate(); });

    _undoHandler = GlobalMapModule().signal_postUndo().connect(
        sigc::mem_fun(this, &SurfaceInspector::doUpdate));
    _redoHandler = GlobalMapModule().signal_postRedo().connect(
        sigc::mem_fun(this, &SurfaceInspector::doUpdate));

    // Get notified about texture changes
    _textureMessageHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::TextureChanged,
        radiant::TypeListener<radiant::TextureChangedMessage>(
            sigc::mem_fun(this, &SurfaceInspector::handleTextureChangedMessage)));
}

void SurfaceInspector::disconnectEventHandlers()
{
    GlobalEventManager().findEvent("TogTexLock")->disconnectToggleButton(_texLockButton);

    GlobalRadiantCore().getMessageBus().removeListener(_textureMessageHandler);
    _textureMessageHandler = 0;

    _selectionChanged.disconnect();
    _undoHandler.disconnect();
    _redoHandler.disconnect();
}

void SurfaceInspector::connectButtons()
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

    _manipulators[HSCALE].smaller->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onScale(HSCALE, false); });
    _manipulators[HSCALE].larger->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onScale(HSCALE, true); });
    _manipulators[VSCALE].smaller->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onScale(VSCALE, true); });
    _manipulators[VSCALE].larger->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onScale(VSCALE, false); });

	wxutil::button::connectToCommand(_manipulators[ROTATION].larger, "TexRotateClock");
	wxutil::button::connectToCommand(_manipulators[ROTATION].smaller, "TexRotateCounter");
}

void SurfaceInspector::onScale(const std::string& scaleId, bool larger)
{
    if (_scaleLinkToggle->GetValue())
    {
        auto scaleValue = string::convert<float>(_manipulators[HSCALE].stepEntry->GetValue().ToStdString());
        scaleValue *= larger ? -1 : 1;

        GlobalCommandSystem().executeCommand("TexScale", Vector2(scaleValue, scaleValue));
    }
    else
    {
        if (scaleId == HSCALE)
        {
            GlobalCommandSystem().executeCommand(larger ? "TexScaleLeft" : "TexScaleRight");
        }
        else
        {
            GlobalCommandSystem().executeCommand(larger ? "TexScaleUp" : "TexScaleDown");
        }
    }
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
    box->SetMinSize(wxSize(box->GetCharWidth() * SPINBOX_WIDTH_CHARS, -1));
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
	auto* fitTextureHBox = new wxBoxSizer(wxHORIZONTAL);

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

    _fitTexture.preserveAspectButton->SetMinSize(PixelSize(30, -1));
    _fitTexture.fitButton->SetMinSize(PixelSize(30, -1));

    // Add widgets to the sizer
    auto* widthTimesHeight = new wxBoxSizer(wxHORIZONTAL);

    widthTimesHeight->Add(_fitTexture.width, 1, wxALIGN_CENTER_VERTICAL);
    widthTimesHeight->Add(_fitTexture.x, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 3);
    widthTimesHeight->Add(_fitTexture.height, 1, wxALIGN_CENTER_VERTICAL);

    fitTextureHBox->Add(widthTimesHeight, 1, wxALIGN_CENTER_VERTICAL);
    fitTextureHBox->Add(_fitTexture.preserveAspectButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    fitTextureHBox->Add(_fitTexture.fitButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

    return fitTextureHBox;
}

void SurfaceInspector::createScaleLinkButtons(wxutil::FormLayout& table)
{
    auto scaleLinkSizer = new wxBoxSizer(wxHORIZONTAL);

    _useHorizScale = new wxBitmapButton(this, wxID_ANY,
                                        wxutil::GetLocalBitmap("link_scale_down.png"));
    _useHorizScale->SetToolTip(_("Assign horizontal scale to vertical scale"));
    scaleLinkSizer->Add(_useHorizScale, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 0);

    _useVertScale = new wxBitmapButton(this, wxID_ANY,
                                       wxutil::GetLocalBitmap("link_scale_up.png"));
    _useVertScale->SetToolTip(_("Assign vertical scale to horizontal scale"));
    scaleLinkSizer->Add(_useVertScale, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

    _useHorizScale->Bind(wxEVT_BUTTON, [&](wxCommandEvent& ev) { onHarmoniseScale(true); });
    _useVertScale->Bind(wxEVT_BUTTON, [&](wxCommandEvent& ev) { onHarmoniseScale(false); });

    auto linkToggle = new wxutil::BitmapToggleButton(this,
                                                     wxutil::GetLocalBitmap("link_active.png"),
                                                     wxutil::GetLocalBitmap("link_inactive.png"));
    linkToggle->SetToolTip(_("Linked Scaling: when active, scale changes will affect horizontal "
                             "and vertical values proportionally"));
    linkToggle->SetMaxClientSize(wxSize(35, -1));
    _scaleLinkToggle = linkToggle;
    scaleLinkSizer->Add(_scaleLinkToggle, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

    table.add(scaleLinkSizer);
}

void SurfaceInspector::populateWindow()
{
	wxBoxSizer* dialogVBox = new wxBoxSizer(wxVERTICAL);

	// Create the title label (bold font)
	wxStaticText* topLabel = new wxStaticText(this, wxID_ANY, _(LABEL_PROPERTIES));
	topLabel->SetFont(topLabel->GetFont().Bold());

    // Two-column form layout
	wxutil::FormLayout table(this);

    // Shader entry box
	wxBoxSizer* shaderHBox = new wxBoxSizer(wxHORIZONTAL);
	_shaderEntry = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	_shaderEntry->SetMinSize(wxSize(100, -1));
	_shaderEntry->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(SurfaceInspector::onShaderEntryActivate), NULL, this);
	shaderHBox->Add(_shaderEntry, 1, wxEXPAND);

	// Create the icon button to open the MaterialChooser
    _selectShaderButton = new wxBitmapButton(
        this, wxID_ANY, wxutil::GetLocalBitmap(FOLDER_ICON)
    );
    _selectShaderButton->SetToolTip(
        _("Choose shader using the shader selection dialog"))
    ;
    _selectShaderButton->Connect(
        wxEVT_BUTTON, wxCommandEventHandler(SurfaceInspector::onShaderSelect), NULL, this
    );
    shaderHBox->Add(_selectShaderButton, 0, wxLEFT, 6);

	table.add(_(LABEL_SHADER), shaderHBox);

	// Pack everything into the vbox
	dialogVBox->Add(topLabel, 0, wxEXPAND | wxBOTTOM, 6);
	dialogVBox->Add(table.getSizer(), 0, wxEXPAND | wxLEFT, 18); // 18 pixels left indentation

	// Initial parameter editing rows
    _manipulators[HSHIFT] = createManipulatorRow(_(LABEL_HSHIFT), table, "arrow_left_blue.png",
                                                 "arrow_right_blue.png");
    _manipulators[VSHIFT] = createManipulatorRow(_(LABEL_VSHIFT), table, "arrow_down_blue.png",
                                                 "arrow_up_blue.png");
    _manipulators[ROTATION] = createManipulatorRow(_(LABEL_ROTATION), table, "rotate_cw.png",
                                                   "rotate_ccw.png");

    // Scale rows separated by a line
    table.addFullWidth(new wxStaticLine(this));
    _manipulators[HSCALE] = createManipulatorRow(_(LABEL_HSCALE), table, "hscale_down.png",
                                                 "hscale_up.png");
    createScaleLinkButtons(table);
    _manipulators[VSCALE] = createManipulatorRow(_(LABEL_VSCALE), table, "vscale_down.png",
                                                 "vscale_up.png");

    // ======================== Texture Operations ====================================

	// Create the texture operations label (bold font)
	wxStaticText* operLabel = new wxStaticText(this, wxID_ANY, _(LABEL_OPERATIONS));
	operLabel->SetFont(operLabel->GetFont().Bold());

    // Setup the table with default spacings
	// 5x2 table with 12 pixel hspacing and 6 pixels vspacing
	wxFlexGridSizer* operTable = new wxFlexGridSizer(5, 2, 6, 12);
	operTable->AddGrowableCol(1);

    // Pack label & table into the dialog
    dialogVBox->AddSpacer(6);
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

    _alignTexture.top->SetMinSize(PixelSize(20, -1));
    _alignTexture.bottom->SetMinSize(PixelSize(20, -1));
    _alignTexture.left->SetMinSize(PixelSize(20, -1));
    _alignTexture.right->SetMinSize(PixelSize(20, -1));

	auto* alignTextureBox = new wxGridSizer(1, 4, 0, 6);

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
	_defaultTexScale->SetMinSize(wxSize(50, -1));
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

SurfaceInspector::ManipulatorRow
SurfaceInspector::createManipulatorRow(const std::string& label, wxutil::FormLayout& table,
                                       const std::string& bitmapSmaller,
                                       const std::string& bitmapLarger)
{
	ManipulatorRow manipRow;

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	// Create the entry field
	manipRow.value = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	manipRow.value->SetMinSize(PixelSize(50, -1));
	manipRow.value->Bind(wxEVT_TEXT_ENTER, &SurfaceInspector::onValueEntryActivate, this);

    // Create the nudge buttons
	wxBoxSizer* controlButtonBox = new wxBoxSizer(wxHORIZONTAL);
    manipRow.smaller = new wxutil::ControlButton(this, wxutil::GetLocalBitmap(bitmapSmaller));
    controlButtonBox->Add(manipRow.smaller, 0, wxEXPAND);
    controlButtonBox->AddSpacer(2);
    manipRow.larger = new wxutil::ControlButton(this, wxutil::GetLocalBitmap(bitmapLarger));
    controlButtonBox->Add(manipRow.larger, 0, wxEXPAND);

	// Create the label
	wxStaticText* steplabel = new wxStaticText(this, wxID_ANY, _(LABEL_STEP));

	// Create the entry field
	manipRow.stepEntry = new wxTextCtrl(this, wxID_ANY, "");
    manipRow.stepEntry->SetMinSize(PixelSize(30, -1));

	// Arrange all items in a row
    hbox->Add(manipRow.value, 1, wxALIGN_CENTER_VERTICAL);
    hbox->Add(controlButtonBox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
	hbox->Add(steplabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
	hbox->Add(manipRow.stepEntry, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

	// Pack the hbox into the table
    table.add(label, hbox);

	// Return the filled structure
	return manipRow;
}

void SurfaceInspector::onHarmoniseScale(bool useHorizontal)
{
    auto horizValue = _manipulators[HSCALE].value->GetValue();
    auto vertValue = _manipulators[VSCALE].value->GetValue();

    if (horizValue == vertValue) return; // nothing to do

    if (useHorizontal)
    {
        _manipulators[VSCALE].value->SetValue(horizValue);
    }
    else
    {
        _manipulators[HSCALE].value->SetValue(vertValue);
    }

    emitTexDef();
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
    _updateNeeded = true;
    requestIdleCallback();
}

void SurfaceInspector::onIdle()
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

	bool haveSelection = (selectionInfo.totalCount > 0);

	// If patches or entities are selected, the value entry fields have no meaning
	bool valueSensitivity = (selectionInfo.patchCount == 0 &&
						selectionInfo.totalCount > 0 &&
						selectionInfo.entityCount == 0 &&
						GlobalSelectionSystem().getSelectedFaceCount() == 1);

	_manipulators[HSHIFT].value->Enable(valueSensitivity);
	_manipulators[VSHIFT].value->Enable(valueSensitivity);
	_manipulators[HSCALE].value->Enable(valueSensitivity);
	_manipulators[VSCALE].value->Enable(valueSensitivity);
	_manipulators[ROTATION].value->Enable(valueSensitivity);

    _useHorizScale->Enable(valueSensitivity);
    _useVertScale->Enable(valueSensitivity);

    // The fit widget sensitivity
    _fitTexture.enable(haveSelection);

    _scaleLinkToggle->Enable(haveSelection);

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
	auto* chooser = new MaterialChooser(this, MaterialSelector::TextureFilter::Regular, _shaderEntry);

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

void SurfaceInspector::handleTextureChangedMessage(radiant::TextureChangedMessage& msg)
{
    update();
}

} // namespace ui
