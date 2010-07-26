#ifndef _GTKUTIL_PATHENTRY_H_
#define _GTKUTIL_PATHENTRY_H_

#include "ifc/EditorWidget.h"

#include "FramedWidget.h"
#include <gtkmm/entry.h>

namespace gtkutil
{

/**
 * greebo: A PathEntry can be used to edit a path to a file or a directory.
 * It behaves like an ordinary text entry box, featuring a button to the right 
 * which opens a FileChooser dialog when clicked.
 */
class PathEntry : 
	public EditorWidget
{
protected:

	// The toplevel widget
	Glib::RefPtr<FramedWidgetmm> _topLevel;

	Gtk::Button* _button;

	// The text entry box
	Gtk::Entry* _entry;

public:
	/** 
	 * Construct a new Path Entry. Use the boolean
	 * to specify whether this widget should be used to 
	 * browser for folders or files.
	 */
	PathEntry(bool foldersOnly = false);

	// Editor widget implementation
	virtual void setValue(const std::string& val);
    virtual std::string getValue() const;

	// Returns the text entry widget
	Gtk::Entry& getEntryWidget();

protected:
   
	// gtkutil::Widget implementation
	virtual GtkWidget* _getWidget() const;

private:
	// gtkmm callbacks
	void onBrowseFiles();
	void onBrowseFolders();
};
typedef boost::shared_ptr<PathEntry> PathEntryPtr;

} // namespace gtkutil

#endif /* _GTKUTIL_PATHENTRY_H_ */
