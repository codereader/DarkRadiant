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

#include "filechooser.h"

#include "ifiletypes.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkfilechooser.h>
#include <gtk/gtkfilechooserdialog.h>
#include <gtk/gtkstock.h>

#include "os/path.h"
#include "os/file.h"

#include "messagebox.h"
#include <boost/algorithm/string/predicate.hpp>

std::string file_dialog_show(GtkWidget* parent,
                             bool open,
                             std::string title,
                             const std::string& path,
                             std::string pattern,
                             const std::string& defaultFile) {
	if (pattern.empty()) {
		pattern = "*";
	}

	if (title.empty()) {
		title = open ? "Open File" : "Save File";
	}
	
	GtkWidget* dialog;
	if (open) {
		dialog = gtk_file_chooser_dialog_new(title.c_str(),
		                                     GTK_WINDOW(parent),
		                                     GTK_FILE_CHOOSER_ACTION_OPEN,
		                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		                                     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		                                     NULL);
	}
	else {
		dialog = gtk_file_chooser_dialog_new(title.c_str(),
		                                     GTK_WINDOW(parent),
		                                     GTK_FILE_CHOOSER_ACTION_SAVE,
		                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		                                     GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		                                     NULL);
	}
	
	if (!open) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), defaultFile.c_str());
	}

	// Set the Enter key to activate the default response
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	// Set position and modality of the dialog
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	// Set the default size of the window
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(dialog));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);

	gtk_window_set_default_size(GTK_WINDOW(dialog), w/2, 2*h/3);

	// Convert path to standard and set the folder in the dialog
	std::string sPath = os::standardPath(path);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
	                                    sPath.c_str());

	// Add the filetype masks
	ModuleTypeListPtr typeList = GlobalFiletypes().getTypesFor(pattern);
	for (ModuleTypeList::iterator i = typeList->begin();
	        i != typeList->end();
	        ++i) {
		// Create a GTK file filter and add it to the chooser dialog
		GtkFileFilter* filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, i->filePattern.pattern.c_str());

		std::string combinedName = i->filePattern.name + " ("
		                           + i->filePattern.pattern + ")";
		gtk_file_filter_set_name(filter, combinedName.c_str());
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	// Add a final mask for All Files (*.*)
	GtkFileFilter* allFilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(allFilter, "*.*");
	gtk_file_filter_set_name(allFilter, "All Files (*.*)");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), allFilter);

	// Display the dialog and return the selected filename, or ""
	std::string retName("");
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		retName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}

	// Destroy the dialog and return the value
	gtk_widget_destroy(dialog);
	return retName;
}

char* dir_dialog(GtkWidget* parent, const char* title, const char* path) {
	GtkWidget* dialog = gtk_file_chooser_dialog_new(title,
	                    GTK_WINDOW(parent),
	                    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	                    NULL);

	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	if(!string_empty(path)) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
	}

	char* filename = 0;
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}

	gtk_widget_destroy(dialog);

	return filename;
}

// Display a file chooser
std::string file_dialog(GtkWidget* parent,
                        bool open,
                        const std::string& title,
                        const std::string& path,
                        const std::string& pattern,
                        const std::string& defaultExt,
                        const std::string& defaultFile)
{
	while (1) {
		std::string file = file_dialog_show(parent, open, title, path, pattern, defaultFile);

		// Convert the backslashes to forward slashes
		file = os::standardPath(file);

		// Append the default extension for save operations before checking overwrites
		if (!open											// save operation
		    && !file.empty() 								// valid filename
		    && !defaultExt.empty()							// non-empty default extension
		    && !boost::algorithm::iends_with(file, defaultExt)) // no default extension
		{
			file += defaultExt;
		}

		std::string askTitle = title;
		askTitle += (!file.empty()) ? ": " + file.substr(file.rfind("/")+1) : "";

		// Always return the file for "open" and empty filenames, otherwise check file existence 
		if (open
		    || file.empty()
		    || !file_exists(file.c_str())
		    || gtk_MessageBox(parent,
		                      "The specified file already exists.\nDo you want to replace it?",
		                      askTitle.c_str(),
		                      eMB_NOYES,
		                      eMB_ICONQUESTION) == eIDYES)
		{
			return file;
		}
	}
}
