#include "ScriptWindow.h"

#include "iscript.h"
#include "ui/igroupdialog.h"
#include "iundo.h"
#include "i18n.h"

#include "wxutil/sourceview/SourceView.h"
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "string/replace.h"

namespace ui
{

ScriptWindow::ScriptWindow(wxWindow* parent) :
    DockablePanel(parent),
	_outView(new wxutil::ConsoleView(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	auto vertPane = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
    vertPane->SetMinimumPaneSize(10); // disallow unsplitting

	GetSizer()->Add(vertPane, 1, wxEXPAND);

	// Edit panel has a label and a "run" button
	auto editPanel = new wxPanel(vertPane, wxID_ANY);
	editPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	auto editLabel = new wxStaticText(editPanel, wxID_ANY, _("Python Script Input"));

	auto runButton = new wxButton(editPanel, wxID_ANY, _("Run Script"));
	runButton->Bind(wxEVT_BUTTON, &ScriptWindow::onRunScript, this);

    _view = new wxutil::PythonSourceViewCtrl(editPanel);

	editPanel->GetSizer()->Add(editLabel, 0);
	editPanel->GetSizer()->Add(_view, 1, wxEXPAND);
	editPanel->GetSizer()->Add(runButton, 0);

	// Pack the scrolled textview and the entry box to the vbox
	_outView->Reparent(vertPane);
	_view->Reparent(vertPane);

	vertPane->SplitHorizontally(editPanel, _outView);
	vertPane->SetSashPosition(150);
}

void ScriptWindow::toggle(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage("Script");
}

void ScriptWindow::onRunScript(wxCommandEvent& ev)
{
	// Clear the output window before running
	_outView->Clear();

	// Extract the script from the input window
	std::string scriptString = _view->GetValue().ToStdString();

	if (scriptString.empty()) return;

	UndoableCommand cmd("runScript");

	// wxWidgets on Windows might produce \r\n, these confuse the python interpreter
	string::replace_all(scriptString, "\r\n", "\n");

	// Run the script
	script::ExecutionResultPtr result = GlobalScriptingSystem().executeString(scriptString);

	// Check if the output only consists of whitespace
	std::string output = string::replace_all_copy(result->output, "\n", "");
	string::replace_all(output, "\t", "");
	string::replace_all(output, " ", "");

	if (!result->errorOccurred && output.empty())
	{
		// If no output and no error, print at least _something_
		_outView->appendText(_("OK"), wxutil::ConsoleView::ModeStandard);
	}
	else
	{
		_outView->appendText(result->output, 
			result->errorOccurred ? wxutil::ConsoleView::ModeError : wxutil::ConsoleView::ModeStandard);
	}
}

} // namespace script
