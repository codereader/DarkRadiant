#include "ConsoleView.h"

#include <boost/algorithm/string/replace.hpp>

namespace wxutil
{

ConsoleView::ConsoleView(wxWindow* parent) :
	wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_RICH2),
	_errorAttr(*wxRED),
	_warningAttr(wxColour(128, 128, 0)),
	_standardAttr(*wxBLACK)
{
    _lineBuffer.reserve(512);
}

void ConsoleView::appendText(const std::string& text, TextMode mode)
{
	// The text usually arrives in single characters at a time
	// Directly writing to the wxTextCtrl is awfully slow, so let's do some buffering

	// In case the textmode changes, we need to flush the line
	if (_bufferMode != mode)
	{
        flushLine();
	}

	// Write to the buffer first
	_bufferMode = mode;
	_buffer.append(text);

	// Once we hit a single newline, flush the line
	if (text == "\n")
	{
        flushLine();
	}

    // Request an idle callback on the GUI thread
	requestIdleCallback();
}

void ConsoleView::flushLine()
{
    if (!_buffer.empty())
    {
        std::lock_guard<std::mutex> lock(_lineBufferMutex);

        _lineBuffer.push_back(std::make_pair(_bufferMode, std::string()));
        _lineBuffer.back().second.swap(_buffer);
    }
}

void ConsoleView::onIdle()
{
    flushLine();

    std::lock_guard<std::mutex> lock(_lineBufferMutex);

	if (_lineBuffer.empty()) return;

    for (LineBuffer::value_type& pair : _lineBuffer)
    {
        switch (pair.first)
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
        boost::algorithm::replace_all(pair.second, "\0", "NULL");

        // Insert at the end of the text buffer
        AppendText(pair.second);
    }
	
    _lineBuffer.clear();

    // Scroll to bottom
	ShowPosition(GetLastPosition());
}

} // namespace
