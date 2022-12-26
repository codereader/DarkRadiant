#pragma once

#include <string>
#include <vector>
#include <utility>
#include <mutex>
#include <wx/textctrl.h>
#include "event/SingleIdleCallback.h"

class wxWindow;

namespace wxutil
{

class ConsoleView :
	public wxTextCtrl,
	private SingleIdleCallback
{
public:
	// The text modes determining the colour
	enum TextMode
	{
		ModeStandard,
		ModeWarning,
		ModeError,
	};

private:
	wxTextAttr _errorAttr;
	wxTextAttr _warningAttr;
	wxTextAttr _standardAttr;

	TextMode _bufferMode = ModeStandard;
	std::string _buffer;

    typedef std::vector<std::pair<TextMode, std::string> > LineBuffer;
    LineBuffer _lineBuffer;

    std::mutex _lineBufferMutex;

public:
	ConsoleView(wxWindow* parent);

	// Appends new text to the end of the buffer
	void appendText(const std::string& text, TextMode mode);

protected:
	void onIdle();
    void flushLine();
};

}
