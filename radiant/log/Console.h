#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <gtk/gtktextview.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkwindow.h>

#include "LogDevice.h"

namespace ui {

/** 
 * greebo: The Console class encapsulates a GtkTextView and represents 
 *         the "device", which the LogWriter is writing its output to. 
 *
 *         The Console is a singleton which needs to be constructed and packed
 *         during mainframe construction.
 */
class Console :
	public applog::LogDevice
{
	// The widget for packing into a parent window
	GtkWidget* _scrolled;
	GtkWidget* _textView;

	GtkTextBuffer* _buffer;
	
	// The tags for colouring the output text
	GtkTextTag* errorTag;
	GtkTextTag* warningTag;
	GtkTextTag* standardTag;

	// Private constructor, creates the Gtk structures
	Console();

public:
	/** 
	 * greebo: Static command target for toggling the console.
	 */
	static void toggle();

	/** 
	 * greebo: Returns the widget pointer for packing into a parent container.
	 */
	GtkWidget* getWidget();

	/**
	 * greebo: Writes the given output string to the Console.
	 * The log level indicates which tag is used for colouring the output.
	 * (Note: this gets called by the LogWriter automatically).
	 */
	void writeLog(const std::string& outputStr, applog::ELogLevel level);

	/**
	 * greebo: Detaches itself from the LogWriter
	 */ 
	void shutdown();

	// Accessor to the static singleton instance.
	static Console& Instance();

private:
	// Static GTK callbacks (ported from GtkRadiant code)
	static gboolean destroy_set_null(GtkWindow* widget, GtkWidget** p);
	static void onClearConsole(GtkMenuItem* menuitem, Console* self);
	static void console_populate_popup(GtkTextView* textview, GtkMenu* menu, Console* self);
};

} // namespace ui

#endif /* _CONSOLE_H_ */
