#ifndef _GTKUTIL_PATHENTRY_H_
#define _GTKUTIL_PATHENTRY_H_

#include "FramedWidget.h"

#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>

namespace gtkutil
{

/**
 * greebo: A PathEntry can be used to edit a path to a file or a directory.
 * It behaves like an ordinary text entry box, featuring a button to the right
 * which opens a FileChooser dialog when clicked.
 */
class PathEntry :
	public Gtk::Frame
{
protected:
	// Browse button
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

	// get/set selected path
	void setValue(const std::string& val);
    std::string getValue() const;

	// Returns the text entry widget
	Gtk::Entry& getEntryWidget();

private:
	// gtkmm callbacks
	void onBrowseFiles();
	void onBrowseFolders();
};

} // namespace gtkutil

#endif /* _GTKUTIL_PATHENTRY_H_ */
