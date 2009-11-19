#ifndef _GTKUTIL_FILECHOOSER_H_
#define _GTKUTIL_FILECHOOSER_H_

// GTK+ file-chooser dialogs.

#include "ifilechooser.h"
#include <string>
#include <boost/shared_ptr.hpp>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkFileChooser GtkFileChooser;

namespace gtkutil
{

class FileChooser :
	public ui::IFileChooser
{
private:
	// Parent widget
	GtkWidget* _parent;

	GtkWidget* _dialog;

	// Window title
	std::string _title;

	std::string _path;
	std::string _file;

	std::string _pattern;

	std::string _defaultExt;

	// Open or save dialog
	bool _open;

	// The optional preview object
	PreviewPtr _preview;

public:
	/**
	 * Construct a new filechooser with the given parameters.
	 *
	 * @parent: The parent GtkWidget
	 * @title: The dialog title.
	 * @open: if TRUE this is asking for "Open" files, FALSE generates a "Save" dialog.
	 * @browseFolders: if TRUE the dialog is asking the user for directories only.
	 * @pattern: the type "map", "prefab", this determines the file extensions.
	 * @defaultExt: The default extension appended when the user enters 
	 *              filenames without extension.
 	 */
	FileChooser(GtkWidget* parent, 
				const std::string& title, 
				bool open, 
				bool browseFolders,
				const std::string& pattern = "",
				const std::string& defaultExt = "");

	virtual ~FileChooser();

	// Lets the dialog start at a certain path
	void setCurrentPath(const std::string& path);

	// Pre-fills the currently selected file
	void setCurrentFile(const std::string& file);

	/**
	 * FileChooser in "open" mode (see constructor) can have one
	 * single preview attached to it. The Preview object will
	 * get notified on selection changes to update the widget it provides.
	 */
	void attachPreview(const PreviewPtr& preview);

	/**
	 * Returns the selected filename (default extension
	 * will be added if appropriate).
	 */
	virtual std::string getSelectedFileName();

	/**
	 * greebo: Displays the dialog and enters the GTK main loop.
	 * Returns the filename or "" if the user hit cancel.
	 *
	 * The returned file name is normalised using the os::standardPath() method.
	 */
	virtual std::string display();

	// Public function for Preview objects. These must set the "active" state
	// of the preview when the onFileSelectionChange() signal is emitted.
	void setPreviewActive(bool active);

private:
	// GTK callback for updating the preview widget
	static void onUpdatePreview(GtkFileChooser* chooser, FileChooser* self);
};

} // namespace gtkutil

#endif /* _GTKUTIL_FILECHOOSER_H_ */
