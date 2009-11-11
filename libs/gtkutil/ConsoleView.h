#ifndef _CONSOLE_VIEW_H_
#define _CONSOLE_VIEW_H_

#include <string>
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/ifc/Widget.h"
#include <gtk/gtkwidget.h>
#include <gtk/gtktextbuffer.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>

namespace gtkutil
{

/**
 * greebo: A console view provides a scrolled textview with a backend
 * GtkTextBuffer plus some convenient methods to write additional text
 * to that buffer. The ConsoleView will take care that the new text is 
 * always "on screen".
 *
 * There are three "modes" available for writing text: STD, WARNING, ERROR
 */
class ConsoleView :
	public gtkutil::Widget
{
	GtkWidget* _scrolledFrame;

	GtkWidget* _textView;
	GtkTextBuffer* _buffer;

	// The tags for colouring the output text
	GtkTextTag* _errorTag;
	GtkTextTag* _warningTag;
	GtkTextTag* _standardTag;

	GtkTextMark* _end;

public:
	ConsoleView();
	virtual ~ConsoleView() {}

	// The text modes determining the colour
	enum ETextMode
	{
		STANDARD,
		WARNING,
		ERROR,
	};

	// Appends new text to the end of the buffer
	void appendText(const std::string& text, ETextMode mode);
	
	// Clears the text buffer
	void clear();

protected:
	// gtkutil::Widget implementation
	GtkWidget* _getWidget() const
	{
		return _scrolledFrame;
	}

private:
	// Static GTK callbacks
	static void onClearConsole(GtkMenuItem* menuitem, ConsoleView* self);
	static void onPopulatePopup(GtkTextView* textview, GtkMenu* menu, ConsoleView* self);
};

} // namespace gtkutil

#endif /* _CONSOLE_VIEW_H_ */
