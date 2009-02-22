#ifndef _COMMAND_ENTRY_H_
#define _COMMAND_ENTRY_H_

#include <list>
#include <string>

#include <gtk/gtkentry.h>

namespace ui {

/**
 * greebo: This class encapsulates a Command entry field, which can be used
 * to issue statements to DarkRadiant's command system.
 *
 * The entry field supports a basic "command history" functionality to retrieve 
 * previously typed commands. The buffer is of variable size, default is 100 commands.
 *
 * Use the GtkWidget* cast operator to pack this element into a parent container.
 */
class CommandEntry
{
	// The default history size
	static const std::size_t DEFAULT_HISTORY_SIZE;

	// The main widget to pack this into parent containers
	GtkWidget* _mainWidget;

	// The list of previously typed commands
	typedef std::list<std::string> History;
	History _history;

	// The maximum number of commands in the history
	std::size_t _historySize;

	// The actual entry box
	GtkWidget* _entry;

	// The current history cursor, 
	// 0 is the currently edited command,
	// 1 is the most recently entered one, larger values are further in the past
	std::size_t _curHistoryIndex;

	// The entry which is currently edited (the most present one)
	// This is remembered when moving to past entries, so that moving back
	// to the present one is not clearing the current input.
	std::string _presentEntry;

public:
	// Constructor is creating widgets
	CommandEntry();

	void setHistorySize(std::size_t size);

	// Retrieve the widget to pack this into a parent container
	operator GtkWidget*();

private:
	// Truncates the history if the number of items exceeds _historySize
	void ensureMaxHistorySize();

	// Returns a historic entry (0 = "present" entry, 1 = first historic entry, ...)
	std::string getHistoricEntry(std::size_t historyIndex);

	void historyMoveTowardsPast();
	void historyMoveTowardsPresent();

	void executeCurrentStatement();

	// GTK callbacks
	static void onCmdEntryActivate(GtkEntry* entry, CommandEntry* self);
	static gboolean onCmdEntryKeyPress(GtkWidget* widget, GdkEventKey* event, CommandEntry* self);
	static void onGoButtonClicked(GtkWidget* button, CommandEntry* self);
};

} // namespace ui

#endif /* _COMMAND_ENTRY_H_ */
