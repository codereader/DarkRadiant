#pragma once

#include "icommandsystem.h"

#include <wx/panel.h>

#include "wxutil/ConsoleView.h"
#include "ui/common/CommandEntry.h"
#include "LogDevice.h"

namespace gtkutil { class ConsoleView; }

namespace ui
{

class Console;
typedef std::shared_ptr<Console> ConsolePtr;

/**
 * greebo: The Console class encapsulates a GtkTextView and represents
 *         the "device", which the LogWriter is writing its output to.
 *
 *         The Console is a singleton which needs to be constructed and packed
 *         during mainframe construction.
 */
class Console :
	public wxPanel,
	public applog::LogDevice
{
private:
	wxutil::ConsoleView* _view;

	// The entry box for console commands
	CommandEntry* _commandEntry;

public:
	/**
	 * Creates a new Console instance, ready for packing into
	 * a sizer/notebook. The caller is responsible for deleting
	 * the object - usually, when packed into a wxWindow, the wxWidgets
	 * framework is taking care of that.
	 */
	Console(wxWindow* parent);

	virtual ~Console();

	/**
	 * greebo: Static command target for toggling the console.
	 */
	static void toggle(const cmd::ArgumentList& args);

	// Command target to clear the console
	void clearCmd(const cmd::ArgumentList& args);

	/**
	 * greebo: Writes the given output string to the Console.
	 * The log level indicates which tag is used for colouring the output.
	 * (Note: this gets called by the LogWriter automatically).
	 */
	void writeLog(const std::string& outputStr, applog::ELogLevel level);

private:
	void shutdown();
};

} // namespace ui
