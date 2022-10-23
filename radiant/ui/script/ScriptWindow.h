#pragma once

#include "wxutil/ConsoleView.h"
#include "wxutil/DockablePanel.h"

class wxCommandEvent;
namespace wxutil { class PythonSourceViewCtrl; }

namespace ui
{

class ScriptWindow :
	public wxutil::DockablePanel
{
private:
	// Use a standard console window for the script output
	wxutil::ConsoleView* _outView;

	wxutil::PythonSourceViewCtrl* _view;

public:
	ScriptWindow(wxWindow* parent);

private:
	void onRunScript(wxCommandEvent& ev);
};

} // namespace
