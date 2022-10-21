#pragma once

#include "wxutil/XmlResourceBasedWidget.h"

#include "wxutil/DockablePanel.h"

class wxFileDirPickerEvent;
class wxSpinDoubleEvent;
class wxSpinCtrlDouble;

namespace ui
{

/**
 * Panel to configure the background image overlay options for the ortho view.
 */
class OrthoBackgroundPanel :
	public wxutil::DockablePanel,
	private wxutil::XmlResourceBasedWidget
{
private:
	wxSpinCtrlDouble* _spinScale;
	wxSpinCtrlDouble* _spinHorizOffset;
	wxSpinCtrlDouble* _spinVertOffset;

	// TRUE, if a widget update is in progress (to avoid callback loops)
	bool _callbackActive;

public:
	OrthoBackgroundPanel(wxWindow* parent);

private:
	// Widget construction helpers
	void setupDialog();

	void initialiseWidgets();
	void updateSensitivity();

	// callbacks
	void _onFileSelection(wxFileDirPickerEvent& ev);
	void _onToggleUseImage(wxCommandEvent& ev);
	void _onOptionToggled(wxCommandEvent& ev);
	void _onScrollChange(wxScrollEvent& ev);
	void _onSpinChange(wxSpinDoubleEvent& ev);
};

}
