#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <gtk/gtktextview.h>
#include <gtk/gtkmenuitem.h>
#include <string>
#include "LogLevels.h"

namespace ui {

/** 
 * greebo: The Console class encapsulates a GtkTextView represents 
 *         the "device", which the various stream buffers are writing to. 
 *         The Console is a singleton which needs to be constructed and packed
 *         during mainframe construction.
 */
class Console {
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
	 * greebo: Returns the widget pointer for packing into a parent container.
	 */
	GtkWidget* getWidget();

	/**
	 * greebo: Writes the given output string <str> to the Console.
	 *         The log level indicates which tag is used for colouring the output.
	 */
	void write(const std::string& str, applog::ELogLevel level);

	/**
	 * greebo: Destroys the text buffer and clears the pointers. Subsequent
	 *         calls to getTextView() will return NULL.
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
