#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <gtk/gtktextview.h>

namespace ui {

/** 
 * greebo: The Console class encapsulates a GtkTextView represents 
 *         the "device", which the various stream buffers are writing to. 
 *         The Console is a singleton which needs to be constructed and packed
 *         during mainframe construction.
 */
class Console {
	// The GTK textview
	GtkWidget* _textView;

	// Private constructor
	Console();

public:
	/**
	 * greebo: Constructs the textview and returns the widget for 
	 *         packing the view into a parent container.
	 */
	GtkWidget* construct();

	/**
	 * greebo: Returns the textview widget pointer or NULL if not constructed yet.
	 */
	GtkWidget* getTextView();

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
	static void console_clear();
	static void console_populate_popup(GtkTextView* textview, GtkMenu* menu, gpointer user_data);
};

} // namespace ui

#endif /* _CONSOLE_H_ */
