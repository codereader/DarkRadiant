#ifndef _COMMAND_ENTRY_H_
#define _COMMAND_ENTRY_H_

#include <list>
#include <string>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkEntry GtkEntry;

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

public:
	// Constructor is creating widgets
	CommandEntry();

	void setHistorySize(std::size_t size);

	// Retrieve the widget to pack this into a parent container
	operator GtkWidget*();

private:
	// Truncates the history if the number of items exceeds _historySize
	void ensureMaxHistorySize();

	// GTK callbacks
	static void onCmdEntryActivate(GtkEntry* entry, CommandEntry* self);
};

} // namespace ui

#endif /* _COMMAND_ENTRY_H_ */
