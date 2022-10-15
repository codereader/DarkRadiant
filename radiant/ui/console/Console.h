#pragma once

#include "icommandsystem.h"
#include "ilogwriter.h"

#include <wx/panel.h>

#include "wxutil/ConsoleView.h"
#include "CommandEntry.h"

namespace radiant { class ClearConsoleMessage; }

namespace ui
{

/**
 * greebo: The Console class encapsulates a wxutil::ConsoleView and represents
 * the "device", which the LogWriter is writing its output to.
 */
class Console :
	public wxPanel,
	public applog::ILogDevice
{
private:
	wxutil::ConsoleView* _view;

	// The entry box for console commands
	CommandEntry* _commandEntry;

    std::size_t _clearConsoleHandler;

public:
	/**
	 * Creates a new Console instance, ready for packing into
	 * a sizer/notebook. The caller is responsible for deleting
	 * the object - usually, when packed into a wxWindow, the wxWidgets
	 * framework is taking care of that.
	 */
	Console(wxWindow* parent);

	~Console() override;

	/**
	 * greebo: Static command target for toggling the console.
	 */
	static void toggle(const cmd::ArgumentList& args);

	void clear(radiant::ClearConsoleMessage& msg);

	/**
	 * greebo: Writes the given output string to the Console.
	 * The log level indicates which tag is used for colouring the output.
	 * (Note: this gets called by the LogWriter automatically).
	 */
	void writeLog(const std::string& outputStr, applog::LogLevel level) override;

	bool isConsole() const override
	{
		return true; // Receive the buffered log output when attached
	}

    void SetFocus() override;
};

} // namespace ui
