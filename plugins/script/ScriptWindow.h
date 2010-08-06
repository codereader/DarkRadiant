#ifndef SCRIPT_WINDOW_H_
#define SCRIPT_WINDOW_H_

#include "icommandsystem.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtktextbuffer.h>
#include "gtkutil/ConsoleView.h"
#include "gtkutil/SourceView.h"

namespace script
{

class ScriptWindow
{
	// The widget for packing into a parent window
	GtkWidget* _vbox;

	// Use a standard console window for the script output
	gtkutil::ConsoleView _outView;

	gtkutil::SourceView* _view;
		
	// Private Constructor
	ScriptWindow();

public:
	/** 
	 * greebo: Static command target for toggling the script window.
	 */
	static void toggle(const cmd::ArgumentList& args);

	/** 
	 * greebo: Returns the widget pointer for packing into a parent container.
	 */
	GtkWidget* getWidget();

	// Accessor to the static singleton instance.
	static ScriptWindow& Instance();

private:
	static void onRunScript(GtkWidget* button, ScriptWindow* self);
};

} // namespace script

#endif /* SCRIPT_WINDOW_H_ */
