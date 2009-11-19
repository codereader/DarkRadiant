#ifndef _GTKUTIL_PATHENTRY_H_
#define _GTKUTIL_PATHENTRY_H_

#include "ifc/EditorWidget.h"

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
	GtkWidget* _topLevel;

	// The text entry box
	GtkWidget* _entry;

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
	GtkWidget* getEntryWidget() const;

protected:
   
	// gtkutil::Widget implementation
	virtual GtkWidget* _getWidget() const;

private:
	// GTK callbacks
	static void onBrowseFiles(GtkWidget* button, PathEntry* self);
	static void onBrowseFolders(GtkWidget* button, PathEntry* self);
};
typedef boost::shared_ptr<PathEntry> PathEntryPtr;

} // namespace gtkutil

#endif /* _GTKUTIL_PATHENTRY_H_ */
