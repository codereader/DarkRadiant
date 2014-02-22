#pragma once

#include <list>
#include <string>
#include <wx/panel.h>

class wxTextCtrl;
class wxCommandEvent;
class wxKeyEvent;

namespace ui
{

/**
 * greebo: This class encapsulates a Command entry field, which can be used
 * to issue statements to DarkRadiant's command system.
 *
 * The entry field supports a basic "command history" functionality to retrieve
 * previously typed commands. The buffer is of variable size, default is 100 commands.
 */
class CommandEntry :
	public wxPanel
{
private:
	// The default history size
	static const std::size_t DEFAULT_HISTORY_SIZE;

	// The list of previously typed commands
	typedef std::list<std::string> History;
	History _history;

	// The maximum number of commands in the history
	std::size_t _historySize;

	// The actual entry box
	wxTextCtrl* _entry;

	// The current history cursor,
	// 0 is the currently edited command,
	// 1 is the most recently entered one, larger values are further in the past
	std::size_t _curHistoryIndex;

	// The entry which is currently edited (the most present one)
	// This is remembered when moving to past entries, so that moving back
	// to the present one is not clearing the current input.
	std::string _presentEntry;

	// Which prefix was displayed when TAB was last hit
	std::string _previousCompletionPrefix;
	std::size_t _curCompletionIndex;

public:
	// Constructor is creating widgets
	CommandEntry(wxWindow* parent);

	void setHistorySize(std::size_t size);

private:
	// Truncates the history if the number of items exceeds _historySize
	void ensureMaxHistorySize();

	// Returns a historic entry (0 = "present" entry, 1 = first historic entry, ...)
	std::string getHistoricEntry(std::size_t historyIndex);

	void historyMoveTowardsPast();
	void historyMoveTowardsPresent();

	void executeCurrentStatement();

	void moveAutoCompletion(int direction);

	// wx callbacks
	void onCmdEntryActivate(wxCommandEvent& ev);
	void onCmdEntryKeyPress(wxKeyEvent& ev);
};

} // namespace ui
