#include "ConsoleView.h"

#include <boost/algorithm/string/replace.hpp>

namespace wxutil
{

ConsoleView::ConsoleView(wxWindow* parent) :
	wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_RICH2),
	_errorAttr(*wxRED),
	_warningAttr(wxColour(128, 128, 0)),
	_standardAttr(*wxBLACK)
{}

void ConsoleView::appendText(const std::string& text, TextMode mode)
{
	// The text usually arrives in single characters at a time
	// Directly writing to the wxTextCtrl is awfully slow, so let's do some buffering

	// In case the textmode changes, we need to flush our buffer
	if (_bufferMode != mode)
	{
		flushIdleCallback();
	}

	// Write to the buffer first
	_bufferMode = mode;
	_buffer.append(text);

	// Once we hit a single newline, flush the buffer
	// this should give just enough performance benefit
	if (text == "\n")
	{
		flushIdleCallback();
	}
	else
	{
		requestIdleCallback();
	}
}

void ConsoleView::onIdle()
{
	if (_buffer.empty()) return;

	switch (_bufferMode)
	{
		case ModeStandard:
			SetDefaultStyle(_standardAttr);
			break;
		case ModeWarning:
			SetDefaultStyle(_warningAttr);
			break;
		case ModeError:
			SetDefaultStyle(_errorAttr);
			break;
		default:
			SetDefaultStyle(_standardAttr);
	};

	// Replace NULL characters
	boost::algorithm::replace_all(_buffer, "\0", "NULL");

	// Insert at the end of the text buffer
	AppendText(_buffer);
	ShowPosition(GetLastPosition());

	_buffer.clear();
}

} // namespace
