/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_GTKUTIL_FILECHOOSER_H)
#define INCLUDED_GTKUTIL_FILECHOOSER_H

/// \file
/// GTK+ file-chooser dialogs.

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkFileChooser GtkFileChooser;

#include <string>
#include <boost/shared_ptr.hpp>

/// \brief Prompts the user to browse for a directory.
/// The prompt window will be transient to \p parent.
/// The directory will initially default to \p path, which must be an absolute path.
/// The returned string is allocated with \c g_malloc and must be freed with \c g_free.
char* dir_dialog(GtkWidget *parent, const char* title = "Choose Directory", const char* path = "");

namespace gtkutil {

class FileChooser	
{
public:
	/**
	 * greebo: A Preview class can be attached to a FileChooser (in "open" mode), 
	 * to allow for adding and updating a preview widget to the dialog.
	 * The Preview object must provide two methods, one for retrieving
	 * the preview widget for addition to the dialog, and one
	 * update method which gets called as soon as the dialog emits the 
	 * selection change signal.
	 */
	class Preview
	{
	public:
		// Retrieve the preview widget for packing into the dialog
		virtual GtkWidget* getPreviewWidget() = 0;

		/**
		 * Gets called whenever the user changes the file selection.
		 * Note: this method must call the setPreviewActive() method on the
		 * FileChooser class to indicate whether the widget is active or not.
		 */
		virtual void onFileSelectionChanged(const std::string& newFileName, FileChooser& fileChooser) = 0;
	};
	typedef boost::shared_ptr<Preview> PreviewPtr;

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
	 * @pattern: the type "map", "prefab", this determines the file extensions.
	 * @defaultExt: The default extension appended when the user enters 
	 *              filenames without extension.
 	 */
	FileChooser(GtkWidget* parent, 
				const std::string& title, 
				bool open, 
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

#endif
