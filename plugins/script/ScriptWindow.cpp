#include "ScriptWindow.h"

#include "iscript.h"
#include "igroupdialog.h"
#include "iuimanager.h"
#include "iundo.h"

#include <gtk/gtk.h>
#include "gtkutil/Paned.h"
#include "gtkutil/TextButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/nonmodal.h"

#include <boost/algorithm/string/replace.hpp>

namespace script
{

	namespace 
	{
		const std::string SCRIPT_LANGUAGE_ID("python");
	}

ScriptWindow::ScriptWindow() :
	_vbox(gtk_vbox_new(FALSE, 6)),
	_view(SCRIPT_LANGUAGE_ID, false) // allow editing
{
	// The Sourceview is already contained in a scrolled frame
	_inScrolled = _view.getWidget();

	gtk_container_set_focus_chain(GTK_CONTAINER(_inScrolled), NULL);

	GtkWidget* runButton = gtkutil::TextButton("Run Script");
	g_signal_connect(G_OBJECT(runButton), "clicked", G_CALLBACK(onRunScript), this);

	GtkWidget* buttonBar = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(buttonBar), runButton, FALSE, FALSE, 0);

	GtkWidget* inputVBox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(inputVBox), gtkutil::LeftAlignedLabel("Python Script Input"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(inputVBox), _inScrolled, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(inputVBox), buttonBar, FALSE, FALSE, 0);

	// Pack the scrolled textview and the entry box to the vbox
	gtkutil::Paned paned(gtkutil::Paned::Vertical, inputVBox, _outView.getWidget());

	gtk_box_pack_start(GTK_BOX(_vbox), paned.getWidget(), TRUE, TRUE, 0);
	gtk_widget_show_all(_vbox);
}

void ScriptWindow::toggle(const cmd::ArgumentList& args) {
	GlobalGroupDialog().togglePage("Script");
}

GtkWidget* ScriptWindow::getWidget() {
	return _vbox;
}

ScriptWindow& ScriptWindow::Instance() {
	static ScriptWindow _scriptWindow;
	return _scriptWindow;
}

void ScriptWindow::onRunScript(GtkWidget* button, ScriptWindow* self)
{
	// Clear the output window before running
	self->_outView.clear();

	// Extract the script from the input window
	std::string scriptString = self->_view.getContents();

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
		self->_outView.appendText("OK", gtkutil::ConsoleView::STANDARD);
	}
	else
	{
		self->_outView.appendText(result->output, (result->errorOccurred) ? gtkutil::ConsoleView::ERROR : gtkutil::ConsoleView::STANDARD);
	}
}

} // namespace script
