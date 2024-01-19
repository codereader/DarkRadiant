#include "OrthoBackgroundPanel.h"

#include "i18n.h"
#include "iscenegraph.h"

#include "registry/registry.h"

#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include "OverlayRegistryKeys.h"
#include "util/ScopedBoolLock.h"

namespace ui
{

OrthoBackgroundPanel::OrthoBackgroundPanel(wxWindow* parent): DockablePanel(parent)
{
    setupDialog();
    initialiseWidgets();
}

namespace
{
    constexpr int LABEL_INDENT = 5;

    wxStaticText* makeLabel(wxWindow* parent, const std::string& text) {
        wxStaticText* label = new wxStaticText(parent, wxID_ANY, text);
        label->SetLabelMarkup(text);
        return label;
    }

    void addSliderRow(wxWindow* parent, wxSizer& sizer, const std::string& label,
                      wxSlider* slider, wxSpinCtrlDouble* spinner)
    {
        sizer.Add(makeLabel(parent, label), 0, wxALL, LABEL_INDENT);

        wxBoxSizer* rightPanel = new wxBoxSizer(wxHORIZONTAL);
        rightPanel->Add(slider, 1);
        rightPanel->Add(spinner);
        sizer.Add(rightPanel, 1, wxEXPAND);
    }
}

wxSpinCtrlDouble* OrthoBackgroundPanel::makeSpinner(wxWindow* parent, float min, float max, float increment)
{
    auto* spinner = new wxSpinCtrlDouble(parent, wxID_ANY);
    spinner->SetRange(min, max);
    spinner->SetIncrement(increment);
    spinner->SetDigits(2); // decimal places
    spinner->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent& ev) {
        onSpinChange(ev);
    });

    return spinner;
}

void OrthoBackgroundPanel::setupDialog()
{
    // Top-level vbox
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // "Use image" checkbox
    _cb.useImage = new wxCheckBox(this, wxID_ANY, _("Use background image"));
    _cb.useImage->SetValue(registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));
    _cb.useImage->Connect(
        wxEVT_CHECKBOX, wxCommandEventHandler(OrthoBackgroundPanel::onToggleUseImage), NULL,
        this
    );
    mainSizer->Add(_cb.useImage, 0, wxEXPAND | wxALL, 12);

    // Sub-panel for all the other controls, which will become enabled or disabled based on
    // the state of the main checkbox.
    _controlsPanel = new wxPanel(this);
    auto* cpanelSizer = new wxFlexGridSizer(2, 6, 6);
    cpanelSizer->AddGrowableCol(1);

    // File picker
    _filePicker = new wxFilePickerCtrl(_controlsPanel, wxID_ANY);
    _filePicker->Connect(
        wxEVT_FILEPICKER_CHANGED,
        wxFileDirPickerEventHandler(OrthoBackgroundPanel::onFileSelection), NULL, this
    );
    cpanelSizer->Add(makeLabel(_controlsPanel, "<b>Image file</b>"), 0, wxALL, LABEL_INDENT);
    cpanelSizer->Add(_filePicker, 1, wxEXPAND);

    // Opacity slider
    _slider.opacity = new wxSlider(_controlsPanel, wxID_ANY, 50, 0, 100);
    _slider.opacity->Connect(
        wxEVT_SLIDER, wxScrollEventHandler(OrthoBackgroundPanel::onScrollChange), NULL, this
    );
    cpanelSizer->Add(makeLabel(_controlsPanel, "<b>Opacity</b>"), 0, wxALL, LABEL_INDENT);
    cpanelSizer->Add(_slider.opacity, 1, wxEXPAND);

    // Scale slider and spinner
    _slider.scale = new wxSlider(_controlsPanel, wxID_ANY, 100, 0, 2000);
    _slider.scale->Connect(
        wxEVT_SLIDER, wxScrollEventHandler(OrthoBackgroundPanel::onScrollChange), NULL, this
    );
    _spinScale = makeSpinner(_controlsPanel, 0, 20, 0.01f);
    addSliderRow(_controlsPanel, *cpanelSizer, "<b>Scale</b>", _slider.scale, _spinScale);

    // Horizontal Offset slider and spinner
    _slider.hOffset = new wxSlider(_controlsPanel, wxID_ANY, 0, -2000, 2000);
    _slider.hOffset->Connect(
        wxEVT_SLIDER, wxScrollEventHandler(OrthoBackgroundPanel::onScrollChange), NULL, this
    );
    _spinHorizOffset = makeSpinner(_controlsPanel, -20, 20, 0.01f);
    addSliderRow(
        _controlsPanel, *cpanelSizer, "<b>Horz. offset</b>", _slider.hOffset, _spinHorizOffset
    );

    // Vertical Offset slider and spinner
    _slider.vOffset = new wxSlider(_controlsPanel, wxID_ANY, 0, -2000, 2000);
    _slider.vOffset->Connect(
        wxEVT_SLIDER, wxScrollEventHandler(OrthoBackgroundPanel::onScrollChange), NULL, this
    );
    _spinVertOffset = makeSpinner(_controlsPanel, -20, 20, 0.01f);
    addSliderRow(
        _controlsPanel, *cpanelSizer, "<b>Vert. offset</b>", _slider.vOffset, _spinVertOffset
    );

    // Options checkboxes
    cpanelSizer->Add(makeLabel(_controlsPanel, "<b>Options</b>"), 0, wxALL, LABEL_INDENT);

    auto* checkboxSizer = new wxBoxSizer(wxVERTICAL);
    _cb.keepAspect = new wxCheckBox(_controlsPanel, wxID_ANY, "Keep aspect");
    _cb.scaleWithViewport = new wxCheckBox(_controlsPanel, wxID_ANY, "Zoom with viewport");
    _cb.panWithViewport = new wxCheckBox(_controlsPanel, wxID_ANY, "Pan with viewport");
    checkboxSizer->Add(_cb.keepAspect);
    checkboxSizer->Add(_cb.scaleWithViewport);
    checkboxSizer->Add(_cb.panWithViewport);
    cpanelSizer->Add(checkboxSizer);

    _cb.keepAspect->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { onOptionToggled(); });
    _cb.scaleWithViewport->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { onOptionToggled(); });
    _cb.panWithViewport->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { onOptionToggled(); });

    _controlsPanel->SetSizerAndFit(cpanelSizer);
    mainSizer->Add(_controlsPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    SetSizerAndFit(mainSizer);
}

void OrthoBackgroundPanel::initialiseWidgets()
{
    _cb.useImage->SetValue(registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));

    // Image filename
    _filePicker->SetFileName(wxFileName(GlobalRegistry().get(RKEY_OVERLAY_IMAGE)));

    _slider.opacity->SetValue(registry::getValue<double>(RKEY_OVERLAY_TRANSPARENCY) * 100.0);

    _spinScale->SetValue(registry::getValue<double>(RKEY_OVERLAY_SCALE));
    _spinHorizOffset->SetValue(registry::getValue<double>(RKEY_OVERLAY_TRANSLATIONX));
    _spinVertOffset->SetValue(registry::getValue<double>(RKEY_OVERLAY_TRANSLATIONY));

    _cb.keepAspect->SetValue(registry::getValue<bool>(RKEY_OVERLAY_PROPORTIONAL));
    _cb.scaleWithViewport->SetValue(registry::getValue<bool>(RKEY_OVERLAY_SCALE_WITH_XY));
    _cb.panWithViewport->SetValue(registry::getValue<bool>(RKEY_OVERLAY_PAN_WITH_XY));

    updateSensitivity();
}

void OrthoBackgroundPanel::updateSensitivity()
{
    // If the "Use image" toggle is disabled, desensitise all the other widgets
    _controlsPanel->Enable(_cb.useImage->GetValue());

    assert(_controlsPanel->IsEnabled() == registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));
}

void OrthoBackgroundPanel::onOptionToggled()
{
    if (_callbackActive) return;

    util::ScopedBoolLock lock(_callbackActive);
    registry::setValue<bool>(RKEY_OVERLAY_PROPORTIONAL, _cb.keepAspect->GetValue());
    registry::setValue<bool>(RKEY_OVERLAY_SCALE_WITH_XY, _cb.scaleWithViewport->GetValue());
    registry::setValue<bool>(RKEY_OVERLAY_PAN_WITH_XY, _cb.panWithViewport->GetValue());
}

void OrthoBackgroundPanel::onToggleUseImage(wxCommandEvent& ev)
{
    registry::setValue(RKEY_OVERLAY_VISIBLE, _cb.useImage->GetValue());
    updateSensitivity();

    // Refresh
    GlobalSceneGraph().sceneChanged();
}

void OrthoBackgroundPanel::onFileSelection(wxFileDirPickerEvent& ev)
{
    // Set registry key
    GlobalRegistry().set(
        RKEY_OVERLAY_IMAGE, _filePicker->GetFileName().GetFullPath().ToStdString()
    );

    // Refresh display
    GlobalSceneGraph().sceneChanged();
}

void OrthoBackgroundPanel::onScrollChange(wxScrollEvent& ev)
{
    if (_callbackActive) return;

    util::ScopedBoolLock lock(_callbackActive);

    // Update spin controls on slider change
    _spinScale->SetValue(_slider.scale->GetValue() / 100.0);
    _spinHorizOffset->SetValue(_slider.hOffset->GetValue() / 100.0);
    _spinVertOffset->SetValue(_slider.vOffset->GetValue() / 100.0);

    registry::setValue<double>(RKEY_OVERLAY_TRANSPARENCY, _slider.opacity->GetValue() / 100.0);
    registry::setValue<double>(RKEY_OVERLAY_SCALE, _spinScale->GetValue());
    registry::setValue<double>(RKEY_OVERLAY_TRANSLATIONX, _spinHorizOffset->GetValue());
    registry::setValue<double>(RKEY_OVERLAY_TRANSLATIONY, _spinVertOffset->GetValue());

    // Refresh display
    GlobalSceneGraph().sceneChanged();
}

void OrthoBackgroundPanel::onSpinChange(wxSpinDoubleEvent& ev)
{
    if (_callbackActive) return;

    util::ScopedBoolLock lock(_callbackActive);

    // Update sliders on spin control change
    _slider.scale->SetValue(_spinScale->GetValue()*100);
    _slider.hOffset->SetValue(_spinHorizOffset->GetValue()*100);
    _slider.vOffset->SetValue(_spinVertOffset->GetValue()*100);

    registry::setValue<double>(RKEY_OVERLAY_TRANSPARENCY, _slider.opacity->GetValue() / 100.0);
    registry::setValue<double>(RKEY_OVERLAY_SCALE, _spinScale->GetValue());
    registry::setValue<double>(RKEY_OVERLAY_TRANSLATIONX, _spinHorizOffset->GetValue());
    registry::setValue<double>(RKEY_OVERLAY_TRANSLATIONY, _spinVertOffset->GetValue());

    // Refresh display
    GlobalSceneGraph().sceneChanged();
}

} // namespace ui
