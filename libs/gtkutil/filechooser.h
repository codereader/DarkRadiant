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

#include <string>

/**
 * Display a file chooser dialog.
 */
/*std::string file_dialog(GtkWidget *parent, 
						bool open, 
						const std::string& title, 
						const std::string& path = "", 
						const std::string& pattern = "",
						const std::string& defaultExt = "",
                        const std::string& defaultFile = "");*/


/// \brief Prompts the user to browse for a directory.
/// The prompt window will be transient to \p parent.
/// The directory will initially default to \p path, which must be an absolute path.
/// The returned string is allocated with \c g_malloc and must be freed with \c g_free.
char* dir_dialog(GtkWidget *parent, const char* title = "Choose Directory", const char* path = "");

namespace gtkutil {

class FileChooser	
{
	// Parent widget
	GtkWidget* _parent;

	// Window title
	std::string _title;

	std::string _path;
	std::string _file;

	std::string _pattern;

	std::string _defaultExt;

	// Open or save dialog
	bool _open;

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

	// Lets the dialog start at a certain path
	void setCurrentPath(const std::string& path);

	// Pre-fills the currently selected file
	void setCurrentFile(const std::string& file);

	/**
	 * greebo: Displays the dialog and enters the GTK main loop.
	 * Returns the filename or "" if the user hit cancel.
	 *
	 * The returned file name is normalised using the os::standardPath() method.
	 */
	std::string display();
};

} // namespace gtkutil

#endif
