#ifndef EMStatement_H_
#define EMStatement_H_

#include "ieventmanager.h"

#include "gtk/gtkmenuitem.h"
#include "gtk/gtktoolbutton.h"
#include "gtk/gtkbutton.h"
#include "gdk/gdk.h"

#include "Event.h"

/* greebo: A Statement is an object that executes a command string when invoked.
 * 
 * Trigger the Statement via the execute() method (usually done by the associated accelerator).
 * 
 * Connect the statement to a GtkToolButton / GtkButton / GtkMenuItem via the connectWidget method. 
 */
class Statement :
	public Event
{
private:
	// The statement to execute
	std::string _statement;

	// Whether this Statement reacts on keyup or keydown
	bool _reactOnKeyUp;

	typedef std::map<GtkWidget*, gulong> WidgetList;
	WidgetList _connectedWidgets;
	
public:
	Statement(const std::string& statement, bool reactOnKeyUp = false);

	virtual ~Statement();
	
	// Invoke the registered callback
	virtual void execute();
	
	// Override the derived keyDown/keyUp method
	virtual void keyUp();
	virtual void keyDown();
	
	// Connect the given menuitem/toolbutton to this Statement
	virtual void connectWidget(GtkWidget* widget);
	
	virtual bool empty() const;
	
private:
	// The static GTK callback methods that can be connected to a ToolButton or a MenuItem
	static gboolean onButtonPress(GtkButton* button, Statement* self);
	static gboolean onToolButtonPress(GtkToolButton* toolButton, Statement* self);
	static gboolean onMenuItemClicked(GtkMenuItem* menuitem, Statement* self);

}; // class Statement

#endif /*EMStatement_H_*/
