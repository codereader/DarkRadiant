#ifndef EMCOMMAND_H_
#define EMCOMMAND_H_

#include "ieventmanager.h"
#include "generic/callback.h"

#include "gtk/gtkmenuitem.h"
#include "gtk/gtktoolbutton.h"
#include "gtk/gtkbutton.h"
#include "gdk/gdk.h"

#include "Event.h"

/* greebo: A Command is an object that contains a single callback. 
 * 
 * Trigger the command via the execute() method (usually done by the associated accelerator).
 * 
 * Connect the command to a GtkToolButton / GtkButton / GtkMenuItem via the connectWidget method. 
 */
class Command :
	public Event
{
	// The callback to be performed on execute()
	Callback _callback;

	// Whether this command reacts on keyup or keydown
	bool _reactOnKeyUp;
	
public:
	Command(const Callback& callback, bool reactOnKeyUp = false);
	
	virtual ~Command() {}

	// Invoke the registered callback
	virtual void execute();
	
	// Override the derived keyDown/keyUp method
	virtual void keyUp();
	virtual void keyDown();
	
	// Connect the given menuitem/toolbutton to this Command
	virtual void connectWidget(GtkWidget* widget);
	
	virtual bool empty() const;
	
private:

	// The static GTK callback methods that can be connected to a ToolButton or a MenuItem
	static gboolean onButtonPress(GtkButton* button, gpointer data);
	static gboolean onToolButtonPress(GtkToolButton* toolButton, gpointer data);
	static gboolean onMenuItemClicked(GtkMenuItem* menuitem, gpointer data);

}; // class Command

#endif /*EMCOMMAND_H_*/
