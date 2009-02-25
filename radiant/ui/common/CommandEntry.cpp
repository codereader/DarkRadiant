#include "CommandEntry.h"

#include "icommandsystem.h"
#include "itextstream.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gdk/gdkkeysyms.h>

#include <boost/algorithm/string/case_conv.hpp>

namespace ui {

const std::size_t CommandEntry::DEFAULT_HISTORY_SIZE = 100;

CommandEntry::CommandEntry() :
	_mainWidget(gtk_hbox_new(FALSE, 6)),
	_historySize(DEFAULT_HISTORY_SIZE),
	_entry(gtk_entry_new()),
	_curHistoryIndex(0)
{
	GtkWidget* goButton = gtk_button_new_with_label("Go");
	g_signal_connect(G_OBJECT(goButton), "clicked", G_CALLBACK(onCmdEntryActivate), this);

	// Pack the widget sinto the hbox
	gtk_box_pack_start(GTK_BOX(_mainWidget), _entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_mainWidget), goButton, FALSE, FALSE, 0);

	// Connect the signal
	g_signal_connect(G_OBJECT(_entry), "activate", G_CALLBACK(onCmdEntryActivate), this);
	g_signal_connect(G_OBJECT(_entry), "key-press-event", G_CALLBACK(onCmdEntryKeyPress), this);
}

CommandEntry::operator GtkWidget*() {
	gtk_widget_show_all(_mainWidget);
	return _mainWidget;
}

void CommandEntry::setHistorySize(std::size_t size) {
	_historySize = size;

	ensureMaxHistorySize();
}

void CommandEntry::ensureMaxHistorySize() {
	// Anything to do at all?
	if (_history.size() <= _historySize) {
		return; 
	}

	while (_history.size() > _historySize && _history.size() > 0) {
		_history.pop_back();
	}
}

std::string CommandEntry::getHistoricEntry(std::size_t historyIndex) {
	if (historyIndex == 0) {
		return _presentEntry;
	}

	// Find the n-th entry
	History::const_iterator h = _history.begin();

	for (std::size_t i = 1; i < historyIndex; ++i, ++h) {
		// Check if we exceeded the limits
		if (h == _history.end()) {
			return "";
		}
	}

	return *h;
}

void CommandEntry::historyMoveTowardsPast() {
	// Go back in the history
	if (_history.size() == 0 || _curHistoryIndex == _history.size()) {
		return; // can't go further
	}

	// We have some history entries and the cursor is not in the past yet

	if (_curHistoryIndex == 0) {
		// We're moving from the present to the past, remember this one before moving
		_presentEntry = gtk_entry_get_text(GTK_ENTRY(_entry));
	}

	_curHistoryIndex++;

	std::string histEntry = getHistoricEntry(_curHistoryIndex);
	gtk_entry_set_text(GTK_ENTRY(_entry), histEntry.c_str());
	// Move the cursor to the last position
	gtk_editable_set_position(GTK_EDITABLE(_entry), -1);

	_previousCompletionPrefix.clear();
}

void CommandEntry::historyMoveTowardsPresent() {
	if (_curHistoryIndex == 0) {
		return; // we are already at the present entry
	}

	_curHistoryIndex--;

	std::string histEntry = getHistoricEntry(_curHistoryIndex);
	gtk_entry_set_text(GTK_ENTRY(_entry), histEntry.c_str());
	// Move the cursor to the last position
	gtk_editable_set_position(GTK_EDITABLE(_entry), -1);

	_previousCompletionPrefix.clear();
}

void CommandEntry::executeCurrentStatement() {
	// Reset the history cursor to the last entry
	_curHistoryIndex = 0;
	_presentEntry.clear();

	// Take the contents of the entry box and pass it to the command window
	std::string command = gtk_entry_get_text(GTK_ENTRY(_entry));

	globalOutputStream() << ">> " << command << std::endl;

	if (command.empty()) return; // nothing to do

	// Pass the command string
	GlobalCommandSystem().execute(command);

	// Push this command to the history
	_history.push_front(command);
	ensureMaxHistorySize();

	// Clear the command entry after execution
	gtk_entry_set_text(GTK_ENTRY(_entry), "");
}

void CommandEntry::moveAutoCompletion(int direction) {
	// Get the current prefix
	std::string prefixOrig = gtk_entry_get_text(GTK_ENTRY(_entry));
	std::string prefix = boost::algorithm::to_lower_copy(prefixOrig);

	if (prefix.empty()) {
		_previousCompletionPrefix.clear();
		_curCompletionIndex = 0;
		return;
	}

	// Cut off the selected part
	gint startpos = 0;
	gint endpos = 0;
	if (gtk_editable_get_selection_bounds(GTK_EDITABLE(_entry), &startpos, &endpos)) {
		prefix = prefix.substr(0, startpos);
	}

	cmd::AutoCompletionInfo info = GlobalCommandSystem().getAutoCompletionInfo(prefix);

	if (_previousCompletionPrefix != prefix) {
		// Prefix has changed write a new list of candidates
		globalOutputStream() << ">> " << prefixOrig << std::endl;

		for (cmd::AutoCompletionInfo::Candidates::const_iterator i = info.candidates.begin();
			i != info.candidates.end(); ++i)
		{
			globalOutputStream() << *i << std::endl;
		}
	}

	if (info.candidates.empty()) {
		globalOutputStream() << "No matches for " << prefixOrig << std::endl;
	}
	else {
		// Cycle through candidates
		if (_previousCompletionPrefix != prefix) {
			_curCompletionIndex = 0;
		}
		else {
			_curCompletionIndex++;

			if (_curCompletionIndex >= info.candidates.size()) {
				_curCompletionIndex = 0;
			}
		}

		// Take the n-th completion candidate and append it
		gtk_entry_set_text(GTK_ENTRY(_entry), info.candidates[_curCompletionIndex].c_str());

		// Select the suggested part
		gtk_editable_select_region(GTK_EDITABLE(_entry), static_cast<gint>(prefix.length()), -1);
	}

	_previousCompletionPrefix = prefix;
}

void CommandEntry::onCmdEntryActivate(GtkEntry* entry, CommandEntry* self) {
	self->executeCurrentStatement();

	self->_previousCompletionPrefix.clear();
}

gboolean CommandEntry::onCmdEntryKeyPress(GtkWidget* widget, GdkEventKey* event, CommandEntry* self) {
	// Check for up/down keys
	if (event->keyval == GDK_Up) {
		self->historyMoveTowardsPast();
		return TRUE;
	}
	else if (event->keyval == GDK_Down) {
		self->historyMoveTowardsPresent();
		return TRUE;
	}
	else if (event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab) {
		self->moveAutoCompletion(+1);
		return TRUE;
	}

	return FALSE;
}

void CommandEntry::onGoButtonClicked(GtkWidget* button, CommandEntry* self) {
	self->executeCurrentStatement();
}

} // namespace ui
