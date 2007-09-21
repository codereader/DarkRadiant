#ifndef FLOATINGORTHOVIEW_H_
#define FLOATINGORTHOVIEW_H_

#include "XYWnd.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"

/**
 * A floating version of the XYWnd. These are created by the XYWndManager and 
 * notify the manager when they are destroyed.
 */
class FloatingOrthoView
: public gtkutil::PersistentTransientWindow,
  public XYWnd
{
	// The numeric ID of this window
	int _id;
	
private:
	
	// Post-destroy callback. Inform the XYWndManager of our destruction
	virtual void _postDestroy() {
		GlobalXYWnd().notifyXYWndDestroy(_id);
	}
	
public:
	
	/**
	 * Construct a floating XY window with the given numeric ID (assigned by
	 * the XYWndManager).
	 * 
	 * @param id
	 * Unique ID assigned to this window.
	 * 
	 * @param title
	 * The displayed title for this window (e.g. "XY Front").
	 * 
	 * @param parent
	 * The parent window for which this should be a transient (normally the
	 * mainframe).
	 */
	FloatingOrthoView(int id, const std::string& title, GtkWindow* parent)
	: gtkutil::PersistentTransientWindow(title, parent, false),
	  XYWnd(id),
	  _id(id) 
	{ 
		// Get the GL widget from XYWnd parent, and embed it in a frame in the
		// floating window
		GtkWidget* framedWidget = gtkutil::FramedWidget(XYWnd::getWidget());
		gtk_container_add(GTK_CONTAINER(getWindow()), framedWidget);
		
		// Set the parent window for XYWnd
		XYWnd::setParent(GTK_WINDOW(getWindow()));
	}
};

#endif /*FLOATINGORTHOVIEW_H_*/
