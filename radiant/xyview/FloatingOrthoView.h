#ifndef FLOATINGORTHOVIEW_H_
#define FLOATINGORTHOVIEW_H_

#include "XYWnd.h"

#include <iostream>

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
	
protected:
	
	// Pre-destroy callback, disconnect the XYview from all other systems
	virtual void _preDestroy() {
		// Call destroyXYView before the actual window container is destroyed
		XYWnd::destroyXYView();
		PersistentTransientWindow::_preDestroy();
	}

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
		
		gtk_window_set_type_hint(
	    	GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_NORMAL
	    );

		g_signal_connect(G_OBJECT(getWindow()), "focus-in-event", G_CALLBACK(onFocus), this);
	}

	static gboolean onFocus(GtkWidget* widget, GdkEventFocus* event, FloatingOrthoView* self) {
		// Let the global XYWndManager know about the focus change
		GlobalXYWnd().setActiveXY(self->_id);

		return FALSE;
	}

	/** Overrides the setViewType method of the XYWnd base class.
	 *  Extends the functionality by setting the window title.
	 */
	virtual void setViewType(EViewType viewType) {
		// Invoke the base class method first
		XYWnd::setViewType(viewType);
		
		// Get the title string and emit it to the GtkWindow
		std::string title(getViewTypeTitle(viewType));
		gtk_window_set_title(GTK_WINDOW(getWindow()), title.c_str());
	}
	
	virtual ~FloatingOrthoView() {
		// greebo: Call the destroy method of the subclass, before
		// this class gets destructed, otherwise the virtual overridden
		// methods won't get called anymore.
		if (GTK_IS_WIDGET(getWindow())) {
			destroy();
		}
	}
};

#endif /*FLOATINGORTHOVIEW_H_*/
