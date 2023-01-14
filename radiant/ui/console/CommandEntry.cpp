#include "CommandEntry.h"

#include "i18n.h"
#include "icommandsystem.h"
#include "itextstream.h"

#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>

#include "string/case_conv.h"

namespace ui
{

const std::size_t CommandEntry::DEFAULT_HISTORY_SIZE = 500;

CommandEntry::CommandEntry(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_historySize(DEFAULT_HISTORY_SIZE),
	_entry(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER)),
	_curHistoryIndex(0)
{
	SetSizer(new wxBoxSizer(wxHORIZONTAL));

	wxButton* goButton = new wxButton(this, wxID_ANY, _("Go"));
	goButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(CommandEntry::onCmdEntryActivate), NULL, this);

	//Gtk::Button* goButton = Gtk::manage(new Gtk::Button(_("Go")));
	//goButton->signal_clicked().connect(sigc::mem_fun(*this, &CommandEntry::onCmdEntryActivate));

	// Pack the widgets into the hbox
	GetSizer()->Add(_entry, 1, wxEXPAND);
	GetSizer()->Add(goButton, 0, wxEXPAND);

	//pack_start(*_entry, true, true, 0);
	//pack_start(*goButton, false, false, 0);

	// Connect the signal
	_entry->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(CommandEntry::onCmdEntryActivate), NULL, this);
	_entry->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(CommandEntry::onCmdEntryKeyPress), NULL, this);

	//_entry->signal_key_press_event().connect(sigc::mem_fun(*this, &CommandEntry::onCmdEntryKeyPress), false);
	//show_all();
}

void CommandEntry::setHistorySize(std::size_t size)
{
	_historySize = size;

	ensureMaxHistorySize();
}

void CommandEntry::ensureMaxHistorySize()
{
	// Anything to do at all?
	if (_history.size() <= _historySize)
	{
		return;
	}

	while (_history.size() > _historySize && _history.size() > 0)
	{
		_history.pop_back();
	}
}

std::string CommandEntry::getHistoricEntry(std::size_t historyIndex)
{
	if (historyIndex == 0)
	{
		return _presentEntry;
	}

	// Find the n-th entry
	History::const_iterator h = _history.begin();

	for (std::size_t i = 1; i < historyIndex; ++i, ++h)
	{
		// Check if we exceeded the limits
		if (h == _history.end())
		{
			return "";
		}
	}

	return *h;
}

void CommandEntry::historyMoveTowardsPast()
{
	// Go back in the history
	if (_history.size() == 0 || _curHistoryIndex == _history.size())
	{
		return; // can't go further
	}

	// We have some history entries and the cursor is not in the past yet

	if (_curHistoryIndex == 0)
	{
		// We're moving from the present to the past, remember this one before moving
		_presentEntry = _entry->GetValue();
	}

	_curHistoryIndex++;

	_entry->SetValue(getHistoricEntry(_curHistoryIndex));
	// Move the cursor to the last position
	_entry->SetInsertionPointEnd();

	_previousCompletionPrefix.clear();
}

void CommandEntry::historyMoveTowardsPresent()
{
	if (_curHistoryIndex == 0)
	{
		return; // we are already at the present entry
	}

	_curHistoryIndex--;

	_entry->SetValue(getHistoricEntry(_curHistoryIndex));
	// Move the cursor to the last position
	_entry->SetInsertionPointEnd();

	_previousCompletionPrefix.clear();
}

void CommandEntry::executeCurrentStatement()
{
	// Reset the history cursor to the last entry
	_curHistoryIndex = 0;
	_presentEntry.clear();

	// Take the contents of the entry box and pass it to the command window
	std::string command = _entry->GetValue().ToStdString();

	rMessage() << ">> " << command << std::endl;

	if (command.empty()) return; // nothing to do

	// Pass the command string
	GlobalCommandSystem().execute(command);

	// Push this command to the history
	_history.push_front(command);
	ensureMaxHistorySize();

	// Clear the command entry after execution
	_entry->SetValue("");
}

void CommandEntry::moveAutoCompletion(int direction)
{
	// Get the current prefix
	std::string prefixOrig = _entry->GetValue().ToStdString();
	std::string prefix = string::to_lower_copy(prefixOrig);

	if (prefix.empty())
	{
		_previousCompletionPrefix.clear();
		_curCompletionIndex = 0;
		return;
	}

	// Cut off the selected part
	long startpos = 0;
	long endpos = 0;
	_entry->GetSelection(&startpos, &endpos);

	prefix = prefix.substr(0, startpos);

	cmd::AutoCompletionInfo info = GlobalCommandSystem().getAutoCompletionInfo(prefix);

	if (_previousCompletionPrefix != prefix)
	{
		// Prefix has changed write a new list of candidates
		rMessage() << ">> " << prefixOrig << std::endl;

		for (cmd::AutoCompletionInfo::Candidates::const_iterator i = info.candidates.begin();
			i != info.candidates.end(); ++i)
		{
			rMessage() << *i << std::endl;
		}
	}

	if (info.candidates.empty())
	{
		rMessage() << "No matches for " << prefixOrig << std::endl;
	}
	else
	{
		// Cycle through candidates
		if (_previousCompletionPrefix != prefix)
		{
			_curCompletionIndex = 0;
		}
		else
		{
			_curCompletionIndex++;

			if (_curCompletionIndex >= info.candidates.size())
			{
				_curCompletionIndex = 0;
			}
		}

		// Take the n-th completion candidate and append it
		_entry->SetValue(info.candidates[_curCompletionIndex]);

		// Select the suggested part
		_entry->SetSelection(static_cast<int>(prefix.length()), _entry->GetValue().Length());
	}

	_previousCompletionPrefix = prefix;
}

void CommandEntry::onCmdEntryActivate(wxCommandEvent& ev)
{
	executeCurrentStatement();

	_previousCompletionPrefix.clear();
}

void CommandEntry::onCmdEntryKeyPress(wxKeyEvent& ev)
{
	// Check for up/down keys
	if (ev.GetKeyCode() == WXK_UP)
	{
		historyMoveTowardsPast();
	}
	else if (ev.GetKeyCode() == WXK_DOWN)
	{
		historyMoveTowardsPresent();
	}
	else if (ev.GetKeyCode() == WXK_TAB)
	{
		moveAutoCompletion(+1);
	}
	else
	{
		ev.Skip();
	}
}

} // namespace ui
