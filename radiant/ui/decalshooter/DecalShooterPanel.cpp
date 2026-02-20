#include "DecalShooterPanel.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/checkbox.h>
#include <wx/choice.h>

#include "i18n.h"
#include "imap.h"
#include "ilayer.h"
#include "wxutil/Bitmap.h"
#include "ui/materials/MaterialChooser.h"
#include "ui/materials/MaterialSelector.h"

namespace ui
{

DecalShooterPanel* DecalShooterPanel::_instance = nullptr;

DecalShooterPanel::DecalShooterPanel(wxWindow* parent) :
    DockablePanel(parent),
    _widthCtrl(nullptr),
    _heightCtrl(nullptr),
    _offsetCtrl(nullptr),
    _rotationCtrl(nullptr),
    _randomRotationCheckbox(nullptr),
    _materialEntry(nullptr),
    _browseButton(nullptr),
    _autogroupCheckbox(nullptr),
    _flipCheckbox(nullptr),
    _layerChoice(nullptr)
{
    _instance = this;
    populateWindow();

    _mapEventConnection = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(*this, &DecalShooterPanel::onMapEvent)
    );

    connectToMapRoot();
}

DecalShooterPanel::~DecalShooterPanel()
{
    _mapEventConnection.disconnect();
    _layersChangedConnection.disconnect();

    if (_instance == this)
    {
        _instance = nullptr;
    }
}

DecalShooterPanel* DecalShooterPanel::getInstance()
{
    return _instance;
}

double DecalShooterPanel::getDecalWidth() const
{
    return _widthCtrl ? _widthCtrl->GetValue() : 128.0;
}

double DecalShooterPanel::getDecalHeight() const
{
    return _heightCtrl ? _heightCtrl->GetValue() : 128.0;
}

double DecalShooterPanel::getDecalOffset() const
{
    return _offsetCtrl ? _offsetCtrl->GetValue() : 0.125;
}

double DecalShooterPanel::getDecalRotation() const
{
    return _rotationCtrl ? _rotationCtrl->GetValue() : 0.0;
}

bool DecalShooterPanel::isRandomRotationEnabled() const
{
    return _randomRotationCheckbox ? _randomRotationCheckbox->GetValue() : false;
}

bool DecalShooterPanel::isFlipEnabled() const
{
    return _flipCheckbox ? _flipCheckbox->GetValue() : false;
}

std::string DecalShooterPanel::getDecalMaterial() const
{
    return _materialEntry ? _materialEntry->GetValue().ToStdString() : "textures/common/decal";
}

bool DecalShooterPanel::isAutogroupEnabled() const
{
    return _autogroupCheckbox ? _autogroupCheckbox->GetValue() : false;
}

int DecalShooterPanel::getSelectedLayerId() const
{
    if (!_layerChoice || _layerChoice->GetSelection() == wxNOT_FOUND)
    {
        return -1;
    }

    return static_cast<int>(reinterpret_cast<intptr_t>(_layerChoice->GetClientData(_layerChoice->GetSelection())));
}

void DecalShooterPanel::onDecalCreated(const scene::INodePtr& decalNode)
{
    if (!decalNode)
    {
        return;
    }

    auto mapRoot = GlobalMapModule().getRoot();
    if (!mapRoot)
    {
        return;
    }

    // Do the layer assignment
    int selectedLayerId = getSelectedLayerId();
    if (selectedLayerId >= 0)
    {
        decalNode->moveToLayer(selectedLayerId);
    }

    if (isAutogroupEnabled())
    {
        // Create a new group if we dont have one
        if (!_currentSessionGroup)
        {
            _currentSessionGroup = mapRoot->getSelectionGroupManager().createSelectionGroup();
        }

        _currentSessionGroup->addNode(decalNode);
    }
}

void DecalShooterPanel::resetSessionGroup()
{
    _currentSessionGroup.reset();
}

void DecalShooterPanel::onPanelActivated()
{
    connectToMapRoot();
}

void DecalShooterPanel::onPanelDeactivated()
{
    resetSessionGroup();
}

void DecalShooterPanel::populateWindow()
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(6, 2, 6, 12);
    gridSizer->AddGrowableCol(1);

    wxStaticText* widthLabel = new wxStaticText(this, wxID_ANY, _("Width:"));
    _widthCtrl = new wxSpinCtrlDouble(this, wxID_ANY);
    _widthCtrl->SetRange(1.0, 2048.0);
    _widthCtrl->SetValue(128.0);
    _widthCtrl->SetIncrement(8.0);
    _widthCtrl->SetDigits(1);
    gridSizer->Add(widthLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(_widthCtrl, 1, wxEXPAND);

    wxStaticText* heightLabel = new wxStaticText(this, wxID_ANY, _("Height:"));
    _heightCtrl = new wxSpinCtrlDouble(this, wxID_ANY);
    _heightCtrl->SetRange(1.0, 2048.0);
    _heightCtrl->SetValue(128.0);
    _heightCtrl->SetIncrement(8.0);
    _heightCtrl->SetDigits(1);
    gridSizer->Add(heightLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(_heightCtrl, 1, wxEXPAND);

    wxStaticText* offsetLabel = new wxStaticText(this, wxID_ANY, _("Offset:"));
    _offsetCtrl = new wxSpinCtrlDouble(this, wxID_ANY);
    _offsetCtrl->SetRange(0.0, 16.0);
    _offsetCtrl->SetValue(0.125);
    _offsetCtrl->SetIncrement(0.125);
    _offsetCtrl->SetDigits(3);
    _offsetCtrl->SetToolTip(_("Distance from the surface to prevent z-fighting"));
    gridSizer->Add(offsetLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(_offsetCtrl, 1, wxEXPAND);

    wxStaticText* rotationLabel = new wxStaticText(this, wxID_ANY, _("Rotation:"));

    wxBoxSizer* rotationSizer = new wxBoxSizer(wxHORIZONTAL);
    _rotationCtrl = new wxSpinCtrlDouble(this, wxID_ANY);
    _rotationCtrl->SetRange(-180.0, 180.0);
    _rotationCtrl->SetValue(0.0);
    _rotationCtrl->SetIncrement(15.0);
    _rotationCtrl->SetDigits(1);
    _rotationCtrl->SetToolTip(_("Rotation angle in degrees"));

    _randomRotationCheckbox = new wxCheckBox(this, wxID_ANY, _("Random"));
    _randomRotationCheckbox->SetToolTip(_("Apply a random rotation for each decal"));
    _randomRotationCheckbox->Bind(wxEVT_CHECKBOX, &DecalShooterPanel::onRandomRotationToggled, this);

    rotationSizer->Add(_rotationCtrl, 1, wxEXPAND | wxRIGHT, 4);
    rotationSizer->Add(_randomRotationCheckbox, 0, wxALIGN_CENTER_VERTICAL);

    gridSizer->Add(rotationLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(rotationSizer, 1, wxEXPAND);

    wxStaticText* materialLabel = new wxStaticText(this, wxID_ANY, _("Material:"));

    wxBoxSizer* materialSizer = new wxBoxSizer(wxHORIZONTAL);
    _materialEntry = new wxTextCtrl(this, wxID_ANY, "textures/decals/blood1");
    _materialEntry->SetMinSize(wxSize(120, -1));

    _browseButton = new wxBitmapButton(this, wxID_ANY,
        wxutil::GetLocalBitmap("folder16.png"));
    _browseButton->SetToolTip(_("Choose decal material"));
    _browseButton->Bind(wxEVT_BUTTON, &DecalShooterPanel::onBrowseMaterial, this);

    materialSizer->Add(_materialEntry, 1, wxEXPAND | wxRIGHT, 4);
    materialSizer->Add(_browseButton, 0);

    gridSizer->Add(materialLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(materialSizer, 1, wxEXPAND);

    // Layer choice
    wxStaticText* layerLabel = new wxStaticText(this, wxID_ANY, _("Layer:"));
    _layerChoice = new wxChoice(this, wxID_ANY);
    _layerChoice->SetToolTip(_("Assign created decals to this layer"));
    populateLayerChoice();
    gridSizer->Add(layerLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(_layerChoice, 1, wxEXPAND);

    GetSizer()->Add(gridSizer, 0, wxEXPAND | wxALL, 12);

    // Checkboxes row
    wxBoxSizer* checkboxSizer = new wxBoxSizer(wxHORIZONTAL);

    _autogroupCheckbox = new wxCheckBox(this, wxID_ANY, _("Autogroup"));
    _autogroupCheckbox->SetToolTip(_("When enabled, all decals placed during this tool session will be grouped together"));
    _autogroupCheckbox->Bind(wxEVT_CHECKBOX, &DecalShooterPanel::onAutogroupToggled, this);
    checkboxSizer->Add(_autogroupCheckbox, 0, wxRIGHT, 12);

    _flipCheckbox = new wxCheckBox(this, wxID_ANY, _("Flip"));
    _flipCheckbox->SetToolTip(_("Flip the decal. Useful for decals facing the wrong direction."));
    checkboxSizer->Add(_flipCheckbox, 0);

    GetSizer()->Add(checkboxSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    wxStaticText* helpText = new wxStaticText(this, wxID_ANY,
        _("Use Ctrl+Shift+Middle-Click\nin the 3D view to place decals."));
    helpText->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    GetSizer()->Add(helpText, 0, wxALL, 12);
}

void DecalShooterPanel::onBrowseMaterial(wxCommandEvent& ev)
{
    auto* chooser = new MaterialChooser(this, MaterialSelector::TextureFilter::Regular, _materialEntry);
    chooser->ShowModal();
    chooser->Destroy();
}

void DecalShooterPanel::onAutogroupToggled(wxCommandEvent& ev)
{
    if (!_autogroupCheckbox->GetValue())
    {
        resetSessionGroup();
    }
}

void DecalShooterPanel::onRandomRotationToggled(wxCommandEvent& ev)
{
    // Disable the rotation text field when randomizer is enabled
    if (_rotationCtrl)
    {
        _rotationCtrl->Enable(!_randomRotationCheckbox->GetValue());
    }
}

void DecalShooterPanel::populateLayerChoice()
{
    if (!_layerChoice)
    {
        return;
    }

    int previousSelection = getSelectedLayerId();

    _layerChoice->Clear();
    _layerChoice->Append(_("None"), reinterpret_cast<void*>(static_cast<intptr_t>(-1)));

    auto mapRoot = GlobalMapModule().getRoot();
    if (mapRoot)
    {
        mapRoot->getLayerManager().foreachLayer([this](int layerId, const std::string& layerName)
        {
            _layerChoice->Append(layerName, reinterpret_cast<void*>(static_cast<intptr_t>(layerId)));
        });
    }

    if (previousSelection >= 0)
    {
        for (unsigned int i = 0; i < _layerChoice->GetCount(); ++i)
        {
            int layerId = static_cast<int>(reinterpret_cast<intptr_t>(_layerChoice->GetClientData(i)));
            if (layerId == previousSelection)
            {
                _layerChoice->SetSelection(i);
                return;
            }
        }
    }

    _layerChoice->SetSelection(0);
}

void DecalShooterPanel::onLayersChanged()
{
    populateLayerChoice();
}

void DecalShooterPanel::connectToMapRoot()
{
    _layersChangedConnection.disconnect();

    auto mapRoot = GlobalMapModule().getRoot();
    if (mapRoot)
    {
        _layersChangedConnection = mapRoot->getLayerManager().signal_layersChanged().connect(
            sigc::mem_fun(*this, &DecalShooterPanel::onLayersChanged)
        );
    }

    populateLayerChoice();
}

void DecalShooterPanel::onMapEvent(IMap::MapEvent ev)
{
    if (ev == IMap::MapLoaded)
    {
        connectToMapRoot();
    }
    else if (ev == IMap::MapUnloading)
    {
        _layersChangedConnection.disconnect();
        resetSessionGroup();
    }
}

} // namespace ui
