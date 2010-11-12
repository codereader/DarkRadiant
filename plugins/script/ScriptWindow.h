#ifndef SCRIPT_WINDOW_H_
#define SCRIPT_WINDOW_H_

#include "icommandsystem.h"

#include <gtkmm/box.h>
#include "gtkutil/ConsoleView.h"
#include "gtkutil/SourceView.h"

namespace script
{

class ScriptWindow;
typedef boost::shared_ptr<ScriptWindow> ScriptWindowPtr;

class ScriptWindow :
	public Gtk::VBox
{
private:
	// Use a standard console window for the script output
	gtkutil::ConsoleView* _outView;

	gtkutil::SourceView* _view;

	// Private Constructor
	ScriptWindow();
public:
	// Creates/destroys the singleton instance
	static void create();
	static void destroy();

	/**
	 * greebo: Static command target for toggling the script window.
	 */
	static void toggle(const cmd::ArgumentList& args);

	// Accessor to the static singleton instance.
	static ScriptWindowPtr& InstancePtr();

private:
	void onRunScript();
};

} // namespace script

#endif /* SCRIPT_WINDOW_H_ */
