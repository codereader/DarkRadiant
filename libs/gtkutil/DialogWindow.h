#ifndef DIALOGWINDOW_H_
#define DIALOGWINDOW_H_

#include "window/PersistentTransientWindow.h"

namespace gtkutil {

/**
 * A heap-allocated subclass of PersistentTransientWindow which self-destructs
 * when the underlying window is destroyed.
 * 
 * @deprecated
 * Use PersistentTransientWindow or BlockingTransientWindow instead.
 * 
 * @todo 
 * Remove this class and rewrite the two classes which depend on it
 * (ui::FindAndReplaceShader and ui::MapInfoDialog).
 */
class DialogWindow :
	public PersistentTransientWindow
{
private:

	// Self-destroy object after window destruction
	virtual void _postDestroy() {
		delete this;
	}

public:

	// Constructors
	DialogWindow(const std::string& title, GtkWindow* parent) :
		PersistentTransientWindow(title, parent)
	{ }
	
	virtual void populateWindow() = 0;
	
	void setWindowSize(const unsigned int width, 
							   const unsigned int height) 
	{
		gtk_window_set_default_size(GTK_WINDOW(getWindow()), width, height);
	}

}; // class DialogWindow 

} // namespace ui

#endif /*DIALOGWINDOW_H_*/
