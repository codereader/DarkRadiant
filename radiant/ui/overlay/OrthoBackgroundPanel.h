#pragma once

#include "wxutil/DockablePanel.h"

class wxFileDirPickerEvent;
class wxSpinDoubleEvent;
class wxSpinCtrlDouble;
class wxCheckBox;
class wxSlider;
class wxFilePickerCtrl;

namespace ui
{

/// Panel to configure the background image overlay options for the ortho view.
class OrthoBackgroundPanel: public wxutil::DockablePanel
{
    // Widgets
    wxSpinCtrlDouble* _spinScale = nullptr;
    wxSpinCtrlDouble* _spinHorizOffset = nullptr;
    wxSpinCtrlDouble* _spinVertOffset = nullptr;
    wxFilePickerCtrl* _filePicker = nullptr;
    wxPanel* _controlsPanel = nullptr; // everything under "Use image" checkbox

    // Sliders
    struct {
        wxSlider* opacity = nullptr;
        wxSlider* hOffset = nullptr;
        wxSlider* vOffset = nullptr;
        wxSlider* scale = nullptr;
    } _slider;

    // Checkboxes
    struct {
        wxCheckBox* useImage = nullptr;
        wxCheckBox* keepAspect = nullptr;
        wxCheckBox* scaleWithViewport = nullptr;
        wxCheckBox* panWithViewport = nullptr;
    } _cb;

    // TRUE, if a widget update is in progress (to avoid callback loops)
    bool _callbackActive = false;

public:
    OrthoBackgroundPanel(wxWindow* parent);

private:
    // Widget construction helpers
    void setupDialog();
    wxSpinCtrlDouble* makeSpinner(wxWindow* parent, float min, float max, float increment);
    void initialiseWidgets();
    void updateSensitivity();

    // callbacks
    void onFileSelection(wxFileDirPickerEvent& ev);
    void onToggleUseImage(wxCommandEvent& ev);
    void onOptionToggled();
    void onScrollChange(wxScrollEvent& ev);
    void onSpinChange(wxSpinDoubleEvent& ev);
};

}
