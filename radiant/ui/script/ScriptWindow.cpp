#include "ScriptWindow.h"

#include "iscript.h"
#include "iundo.h"
#include "i18n.h"

#include "wxutil/sourceview/SourceView.h"
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "registry/registry.h"

#include "string/replace.h"
#include "ui/Documentation.h"

namespace ui
{

namespace
{
    constexpr const char* const RKEY_WINDOW_STATE = "user/ui/scriptWindow/";
}

ScriptWindow::ScriptWindow(wxWindow* parent) :
    DockablePanel(parent),
	_outView(new wxutil::ConsoleView(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

    _paned = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
    _paned->SetMinimumPaneSize(10); // disallow unsplitting

	GetSizer()->Add(_paned, 1, wxEXPAND);

	// Edit panel has a label and a "run" button
	auto editPanel = new wxPanel(_paned, wxID_ANY);
	editPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	auto editLabel = new wxStaticText(editPanel, wxID_ANY, _("Python Script Input"));

    auto buttonPanel = new wxFlexGridSizer(1, 2, 6, 6);
    buttonPanel->AddGrowableCol(1);

	auto runButton = new wxButton(editPanel, wxID_ANY, _("Run Script"));
	runButton->Bind(wxEVT_BUTTON, &ScriptWindow::onRunScript, this);
    buttonPanel->Add(runButton, 0, wxALIGN_LEFT);

    auto scriptReferenceUrl = registry::getValue<std::string>(RKEY_SCRIPT_REFERENCE_URL);

    if (!scriptReferenceUrl.empty())
    {
        auto referenceButton = new wxButton(editPanel, wxID_ANY, _("Open Script Reference"));
        referenceButton->Bind(wxEVT_BUTTON, [] (auto&) { Documentation::OpenScriptReference({}); });
        buttonPanel->Add(referenceButton, 0, wxALIGN_RIGHT);
    }

    _view = new wxutil::PythonSourceViewCtrl(editPanel);

	editPanel->GetSizer()->Add(editLabel, 0);
	editPanel->GetSizer()->Add(_view, 1, wxEXPAND);
	editPanel->GetSizer()->Add(buttonPanel, 0, wxEXPAND);

	// Pack the scrolled textview and the entry box to the vbox
	_outView->Reparent(_paned);
	_view->Reparent(_paned);

	_paned->SplitHorizontally(editPanel, _outView);
	_paned->SetSashPosition(150);

    // Add the initial import statement
    _view->SetValue(fmt::format(R"(import darkradiant as dr

# Enter your script code here. For reference, see
# {0}
# or the scripts/test.py in DarkRadiant installation folder.
)", scriptReferenceUrl));
}

ScriptWindow::~ScriptWindow()
{
    _panedPosition.saveToPath(RKEY_WINDOW_STATE);
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

void ScriptWindow::onPanelActivated()
{
    _panedPosition.connect(_paned);
    _panedPosition.loadFromPath(RKEY_WINDOW_STATE);
}

void ScriptWindow::onPanelDeactivated()
{
    // Save current position and disconnect the tracker to not receive
    // faulty sizes during reconstruction of the parent window
    _panedPosition.saveToPath(RKEY_WINDOW_STATE);
    _panedPosition.disconnect();
}

void ScriptWindow::restoreSettings()
{
    // Find the information stored in the registry
    if (GlobalRegistry().keyExists(RKEY_WINDOW_STATE))
    {
        _panedPosition.loadFromPath(RKEY_WINDOW_STATE);
    }
    else
    {
        // No saved information, apply standard value
        _panedPosition.setPosition(300);
    }
}


} // namespace script
