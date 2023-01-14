#pragma once

#include "wxutil/ConsoleView.h"
#include "wxutil/DockablePanel.h"
#include "wxutil/PanedPosition.h"

class wxCommandEvent;
namespace wxutil { class PythonSourceViewCtrl; }

class wxSplitterWindow;

namespace ui
{

class ScriptWindow :
	public wxutil::DockablePanel
{
private:
	// Use a standard console window for the script output
	wxutil::ConsoleView* _outView;
    wxSplitterWindow* _paned;

	wxutil::PythonSourceViewCtrl* _view;

    wxutil::PanedPosition _panedPosition;

public:
	ScriptWindow(wxWindow* parent);
    ~ScriptWindow() override;

    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
	void onRunScript(wxCommandEvent& ev);
    void restoreSettings();
};

} // namespace
