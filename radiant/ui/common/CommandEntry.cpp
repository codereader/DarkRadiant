#include "CommandEntry.h"

#include "icommandsystem.h"
#include "itextstream.h"

#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gdk/gdkkeysyms.h>

namespace ui {

const std::size_t CommandEntry::DEFAULT_HISTORY_SIZE = 100;

CommandEntry::CommandEntry() :
	_mainWidget(gtk_hbox_new(FALSE, 6)),
	_historySize(DEFAULT_HISTORY_SIZE),
	_entry(gtk_entry_new())
{
	// Pack the entry into the hbox
	gtk_box_pack_start(GTK_BOX(_mainWidget), _entry, TRUE, TRUE, 0);

	// Connect the signal
	g_signal_connect(G_OBJECT(_entry), "activate", G_CALLBACK(onCmdEntryActivate), this);
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

void CommandEntry::onCmdEntryActivate(GtkEntry* entry, CommandEntry* self) {
	// Take the contents of the entry box and pass it to the command window
	std::string command = gtk_entry_get_text(GTK_ENTRY(self->_entry));

	globalOutputStream() << ">> " << command << std::endl;

	if (command.empty()) return; // nothing to do

	// Pass the command string
	GlobalCommandSystem().execute(command);

	// Push this command to the history
	self->_history.push_front(command);
	self->ensureMaxHistorySize();

	// Clear the command entry after execution
	gtk_entry_set_text(GTK_ENTRY(self->_entry), "");
}

} // namespace ui
