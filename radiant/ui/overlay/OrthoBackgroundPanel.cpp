#include "OrthoBackgroundPanel.h"

#include "iscenegraph.h"

#include "registry/registry.h"

#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>

#include "OverlayRegistryKeys.h"
#include "util/ScopedBoolLock.h"

namespace ui
{

OrthoBackgroundPanel::OrthoBackgroundPanel(wxWindow* parent): DockablePanel(parent)
{
    auto panel = loadNamedPanel(this, "OverlayDialogMainPanel");
    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(panel, 1, wxEXPAND);

    setupDialog();
    initialiseWidgets();
}

void OrthoBackgroundPanel::setupDialog()
{
    wxCheckBox* useImageBtn = findNamedObject<wxCheckBox>(this, "OverlayDialogUseBackgroundImage");
    useImageBtn->SetValue(registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));
    useImageBtn->Connect(wxEVT_CHECKBOX,
        wxCommandEventHandler(OrthoBackgroundPanel::onToggleUseImage), NULL, this);

    wxFilePickerCtrl* filepicker = findNamedObject<wxFilePickerCtrl>(this, "OverlayDialogFilePicker");
    filepicker->Connect(wxEVT_FILEPICKER_CHANGED,
        wxFileDirPickerEventHandler(OrthoBackgroundPanel::onFileSelection), NULL, this);

    wxSlider* transSlider = findNamedObject<wxSlider>(this, "OverlayDialogTransparencySlider");
    transSlider->Connect(wxEVT_SLIDER, wxScrollEventHandler(OrthoBackgroundPanel::onScrollChange), NULL, this);

    // Scale slider
    wxSlider* scaleSlider = findNamedObject<wxSlider>(this, "OverlayDialogScaleSlider");
    scaleSlider->Connect(
        wxEVT_SLIDER, wxScrollEventHandler(OrthoBackgroundPanel::onScrollChange), NULL, this
    );
    wxPanel* scalePanel = findNamedObject<wxPanel>(this, "OverlayDialogScalePanel");

    // Scale spinbox
    _spinScale = new wxSpinCtrlDouble(scalePanel, wxID_ANY);
    _spinScale->SetRange(0, 20);
    _spinScale->SetIncrement(0.01);
    _spinScale->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent& ev) {
        onSpinChange(ev);
    });

    scalePanel->GetSizer()->Add(_spinScale, 0, wxLEFT, 6);
    scalePanel->GetSizer()->Layout();

    // Horizontal Offset slider
    wxSlider* hOffsetSlider = findNamedObject<wxSlider>(this, "OverlayDialogHorizOffsetSlider");
    hOffsetSlider->Connect(
        wxEVT_SLIDER, wxScrollEventHandler(OrthoBackgroundPanel::onScrollChange), NULL, this
    );
    wxPanel* hOffsetPanel = findNamedObject<wxPanel>(this, "OverlayDialogHorizOffsetPanel");

    // Horizontal Offset spinbox
    _spinHorizOffset = new wxSpinCtrlDouble(hOffsetPanel, wxID_ANY);
    _spinHorizOffset->SetRange(-20, 20);
    _spinHorizOffset->SetIncrement(0.01);
    _spinHorizOffset->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent& ev) {
        onSpinChange(ev);
    });

    hOffsetPanel->GetSizer()->Add(_spinHorizOffset, 0, wxLEFT, 6);
    hOffsetPanel->GetSizer()->Layout();

    // Vertical Offset slider
    wxSlider* vOffsetSlider = findNamedObject<wxSlider>(this, "OverlayDialogVertOffsetSlider");
    vOffsetSlider->Connect(wxEVT_SLIDER, wxScrollEventHandler(OrthoBackgroundPanel::onScrollChange), NULL, this);
    wxPanel* vOffsetPanel = findNamedObject<wxPanel>(this, "OverlayDialogVertOffsetPanel");

    // Vertical Offset spinbox
    _spinVertOffset = new wxSpinCtrlDouble(vOffsetPanel, wxID_ANY);
    _spinVertOffset->SetRange(-20, 20);
    _spinVertOffset->SetIncrement(0.01);
    _spinVertOffset->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent& ev) {
        onSpinChange(ev);
    });

    vOffsetPanel->GetSizer()->Add(_spinVertOffset, 0, wxLEFT, 6);
    vOffsetPanel->GetSizer()->Layout();

    _cb.keepAspect = findNamedObject<wxCheckBox>(this, "OverlayDialogKeepAspect");
    _cb.scaleWithViewport = findNamedObject<wxCheckBox>(this, "OverlayDialogZoomWithViewport");
    _cb.panWithViewport = findNamedObject<wxCheckBox>(this, "OverlayDialogPanWithViewport");

    _cb.keepAspect->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { onOptionToggled(); });
    _cb.scaleWithViewport->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { onOptionToggled(); });
    _cb.panWithViewport->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { onOptionToggled(); });

    makeLabelBold(this, "OverlayDialogLabelFile");
    makeLabelBold(this, "OverlayDialogLabelTrans");
    makeLabelBold(this, "OverlayDialogLabelScale");
    makeLabelBold(this, "OverlayDialogLabelHOffset");
    makeLabelBold(this, "OverlayDialogLabelVOffset");
    makeLabelBold(this, "OverlayDialogLabelOptions");
}

void OrthoBackgroundPanel::initialiseWidgets()
{
    wxCheckBox* useImageBtn = findNamedObject<wxCheckBox>(this, "OverlayDialogUseBackgroundImage");
    useImageBtn->SetValue(registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));

    // Image filename
    wxFilePickerCtrl* filepicker = findNamedObject<wxFilePickerCtrl>(this, "OverlayDialogFilePicker");
    filepicker->SetFileName(wxFileName(GlobalRegistry().get(RKEY_OVERLAY_IMAGE)));

    wxSlider* transSlider = findNamedObject<wxSlider>(this, "OverlayDialogTransparencySlider");

    transSlider->SetValue(registry::getValue<double>(RKEY_OVERLAY_TRANSPARENCY) * 100.0);

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
    wxCheckBox* useImageBtn = findNamedObject<wxCheckBox>(this, "OverlayDialogUseBackgroundImage");

    wxPanel* controls = findNamedObject<wxPanel>(this, "OverlayDialogControlPanel");
    controls->Enable(useImageBtn->GetValue());

    assert(controls->IsEnabled() == registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));
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
    wxCheckBox* useImageBtn = static_cast<wxCheckBox*>(ev.GetEventObject());

    registry::setValue(RKEY_OVERLAY_VISIBLE, useImageBtn->GetValue());
    updateSensitivity();

    // Refresh
    GlobalSceneGraph().sceneChanged();
}

void OrthoBackgroundPanel::onFileSelection(wxFileDirPickerEvent& ev)
{
    // Set registry key
    wxFilePickerCtrl* filepicker = findNamedObject<wxFilePickerCtrl>(this, "OverlayDialogFilePicker");

    GlobalRegistry().set(RKEY_OVERLAY_IMAGE, filepicker->GetFileName().GetFullPath().ToStdString());

    // Refresh display
    GlobalSceneGraph().sceneChanged();
}

void OrthoBackgroundPanel::onScrollChange(wxScrollEvent& ev)
{
    if (_callbackActive) return;

    _callbackActive = true;

    // Update spin controls on slider change
    wxSlider* transSlider = findNamedObject<wxSlider>(this, "OverlayDialogTransparencySlider");

    wxSlider* scaleSlider = findNamedObject<wxSlider>(this, "OverlayDialogScaleSlider");
    _spinScale->SetValue(scaleSlider->GetValue() / 100.0);

    wxSlider* hOffsetSlider = findNamedObject<wxSlider>(this, "OverlayDialogHorizOffsetSlider");
    _spinHorizOffset->SetValue(hOffsetSlider->GetValue() / 100.0);

    wxSlider* vOffsetSlider = findNamedObject<wxSlider>(this, "OverlayDialogVertOffsetSlider");
    _spinVertOffset->SetValue(vOffsetSlider->GetValue() / 100.0);

    registry::setValue<double>(RKEY_OVERLAY_TRANSPARENCY, transSlider->GetValue() / 100.0);
    registry::setValue<double>(RKEY_OVERLAY_SCALE, _spinScale->GetValue());
    registry::setValue<double>(RKEY_OVERLAY_TRANSLATIONX, _spinHorizOffset->GetValue());
    registry::setValue<double>(RKEY_OVERLAY_TRANSLATIONY, _spinVertOffset->GetValue());

    // Refresh display
    GlobalSceneGraph().sceneChanged();

    _callbackActive = false;
}

void OrthoBackgroundPanel::onSpinChange(wxSpinDoubleEvent& ev)
{
    if (_callbackActive) return;

    _callbackActive = true;

    // Update sliders on spin control change
    wxSlider* transSlider = findNamedObject<wxSlider>(this, "OverlayDialogTransparencySlider");

    wxSlider* scaleSlider = findNamedObject<wxSlider>(this, "OverlayDialogScaleSlider");
    scaleSlider->SetValue(_spinScale->GetValue()*100);

    wxSlider* hOffsetSlider = findNamedObject<wxSlider>(this, "OverlayDialogHorizOffsetSlider");
    hOffsetSlider->SetValue(_spinHorizOffset->GetValue()*100);

    wxSlider* vOffsetSlider = findNamedObject<wxSlider>(this, "OverlayDialogVertOffsetSlider");
    vOffsetSlider->SetValue(_spinVertOffset->GetValue()*100);

    registry::setValue<double>(RKEY_OVERLAY_TRANSPARENCY, transSlider->GetValue() / 100.0);
    registry::setValue<double>(RKEY_OVERLAY_SCALE, _spinScale->GetValue());
    registry::setValue<double>(RKEY_OVERLAY_TRANSLATIONX, _spinHorizOffset->GetValue());
    registry::setValue<double>(RKEY_OVERLAY_TRANSLATIONY, _spinVertOffset->GetValue());

    // Refresh display
    GlobalSceneGraph().sceneChanged();

    _callbackActive = false;
}

} // namespace ui
