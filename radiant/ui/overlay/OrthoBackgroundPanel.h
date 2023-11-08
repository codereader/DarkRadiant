#pragma once

#include "wxutil/XmlResourceBasedWidget.h"

#include "wxutil/DockablePanel.h"

class wxFileDirPickerEvent;
class wxSpinDoubleEvent;
class wxSpinCtrlDouble;

namespace ui
{

/// Panel to configure the background image overlay options for the ortho view.
class OrthoBackgroundPanel :
    public wxutil::DockablePanel,
    private wxutil::XmlResourceBasedWidget
{
    wxSpinCtrlDouble* _spinScale = nullptr;
    wxSpinCtrlDouble* _spinHorizOffset = nullptr;
    wxSpinCtrlDouble* _spinVertOffset = nullptr;

    // TRUE, if a widget update is in progress (to avoid callback loops)
    bool _callbackActive = false;

public:
    OrthoBackgroundPanel(wxWindow* parent);

private:
    // Widget construction helpers
    void setupDialog();

    void initialiseWidgets();
    void updateSensitivity();

    // callbacks
    void onFileSelection(wxFileDirPickerEvent& ev);
    void onToggleUseImage(wxCommandEvent& ev);
    void onOptionToggled(wxCommandEvent& ev);
    void onScrollChange(wxScrollEvent& ev);
    void onSpinChange(wxSpinDoubleEvent& ev);
};

}
