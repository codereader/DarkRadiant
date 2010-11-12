#include "ScriptWindow.h"

#include "iscript.h"
#include "igroupdialog.h"
#include "iuimanager.h"
#include "iundo.h"
#include "i18n.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/nonmodal.h"

#include <gtkmm/button.h>
#include <gtkmm/paned.h>

#include <boost/algorithm/string/replace.hpp>

namespace script
{

	namespace
	{
		const std::string SCRIPT_LANGUAGE_ID("python");
	}

ScriptWindow::ScriptWindow() :
	Gtk::VBox(false, 6),
	_outView(Gtk::manage(new gtkutil::ConsoleView)),
	_view(Gtk::manage(new gtkutil::SourceView(SCRIPT_LANGUAGE_ID, false))) // allow editing
{
	_view->unset_focus_chain();

	Gtk::Button* runButton = Gtk::manage(new Gtk::Button(_("Run Script")));
	runButton->signal_clicked().connect(sigc::mem_fun(*this, &ScriptWindow::onRunScript));

	Gtk::HBox* buttonBar = Gtk::manage(new Gtk::HBox(false, 6));
	buttonBar->pack_start(*runButton, false, false, 0);

	Gtk::VBox* inputVBox = Gtk::manage(new Gtk::VBox(false, 3));
	inputVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Python Script Input"))), false, false, 0);
	inputVBox->pack_start(*_view, true, true, 0);
	inputVBox->pack_start(*buttonBar, false, false, 0);

	// Pack the scrolled textview and the entry box to the vbox
	Gtk::VPaned* paned = Gtk::manage(new Gtk::VPaned);
	paned->add1(*inputVBox);
	paned->add2(*_outView);

	pack_start(*paned, true, true, 0);
	show_all();
}

void ScriptWindow::toggle(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage("Script");
}

ScriptWindowPtr& ScriptWindow::InstancePtr()
{
	static ScriptWindowPtr _scriptWindowPtr;
	return _scriptWindowPtr;
}

void ScriptWindow::create()
{
	assert(InstancePtr() == NULL); // prevent double-creations

	InstancePtr().reset(new ScriptWindow);
}

void ScriptWindow::destroy()
{
	InstancePtr().reset();
}

void ScriptWindow::onRunScript()
{
	// Clear the output window before running
	_outView->clear();

	// Extract the script from the input window
	std::string scriptString = _view->getContents();

	if (scriptString.empty()) return;

	UndoableCommand cmd("runScript");

	// Run the script
	script::ExecutionResultPtr result = GlobalScriptingSystem().executeString(scriptString);

	// Check if the output only consists of whitespace
	std::string output = boost::algorithm::replace_all_copy(result->output, "\n", "");
	boost::algorithm::replace_all(output, "\t", "");
	boost::algorithm::replace_all(output, " ", "");

	if (!result->errorOccurred && output.empty())
	{
		// If no output and no error, print at least _something_
		_outView->appendText(_("OK"), gtkutil::ConsoleView::STANDARD);
	}
	else
	{
		_outView->appendText(result->output, (result->errorOccurred) ? gtkutil::ConsoleView::ERROR : gtkutil::ConsoleView::STANDARD);
	}
}

} // namespace script
