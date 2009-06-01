#include "ScriptWindow.h"

#include "iscript.h"
#include "igroupdialog.h"
#include "iuimanager.h"
#include "iundo.h"

#include <gtk/gtk.h>
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/Paned.h"
#include "gtkutil/TextButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/nonmodal.h"

#include <boost/algorithm/string/replace.hpp>

namespace script
{

ScriptWindow::ScriptWindow() :
	_vbox(gtk_vbox_new(FALSE, 6))
{
	// Create the textview, which acts as textbuffer
	_inTextView = gtk_text_view_new();
	
	// Remember the pointers to the textbuffers
	_inBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_inTextView));

	gtk_widget_set_size_request(_inTextView, 0, -1); // allow shrinking
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_inTextView), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(_inTextView), TRUE);

	_inScrolled = gtkutil::ScrolledFrame(_inTextView);

	widget_connect_escape_clear_focus_widget(_inTextView);

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
	gtk_box_pack_start(GTK_BOX(_vbox), gtkutil::Paned(inputVBox, _outView, false), TRUE, TRUE, 0);
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

	GtkTextIter start;
	GtkTextIter end;

	gtk_text_buffer_get_bounds(self->_inBuffer, &start, &end);

	// Extract the script from the input window
	gchar* text = gtk_text_buffer_get_text(self->_inBuffer, &start, &end, TRUE);
	std::string scriptString(text);
	g_free(text);

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
