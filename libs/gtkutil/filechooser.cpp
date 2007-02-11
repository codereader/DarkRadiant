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

#include <list>
#include <vector>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkfilechooser.h>
#include <gtk/gtkfilechooserdialog.h>
#include <gtk/gtkstock.h>

#include "string/string.h"
#include "stream/stringstream.h"
#include "container/array.h"
#include "os/path.h"
#include "os/file.h"

#include "messagebox.h"

const char* file_dialog_show(GtkWidget* parent, 
							 bool open, 
							 const char* title, 
							 const char* path, 
							 const char* pattern)
{
  if(pattern == 0)
  {
    pattern = "*";
  }

  if (title == 0)
    title = open ? "Open File" : "Save File";
    
  GtkWidget* dialog;
  if (open)
  {
    dialog = gtk_file_chooser_dialog_new(title,
                                        GTK_WINDOW(parent),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);
  }
  else
  {
    dialog = gtk_file_chooser_dialog_new(title,
                                        GTK_WINDOW(parent),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                        NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "unnamed");
  }

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

  // we expect an actual path below, if the path is 0 we might crash
  if (path != 0 && !string_empty(path))
  {
    ASSERT_MESSAGE(path_is_absolute(path), "file_dialog_show: path not absolute: " << makeQuoted(path));

    Array<char> new_path(strlen(path)+1);

    // copy path, replacing dir separators as appropriate
    Array<char>::iterator w = new_path.begin();
    for(const char* r = path; *r != '\0'; ++r)
    {
      *w++ = (*r == '/') ? G_DIR_SEPARATOR : *r;
    }
    // remove separator from end of path if required
    if(*(w-1) == G_DIR_SEPARATOR)
    {
      --w;
    }
    // terminate string
    *w = '\0';

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), new_path.data());
  }

	// Add the filetype masks
	ModuleTypeListPtr typeList = GlobalFiletypes().getTypesFor(pattern);
	for (ModuleTypeList::iterator i = typeList->begin();
		 i != typeList->end();
		 ++i)
	{
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

	// Display the dialog and return the selected filename, or NULL	
	const char* retName = NULL;
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		retName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}

	// Destroy the dialog and return the value
	gtk_widget_destroy(dialog);
	return retName;
}

char* dir_dialog(GtkWidget* parent, const char* title, const char* path)
{
  GtkWidget* dialog = gtk_file_chooser_dialog_new(title,
				        GTK_WINDOW(parent),
				        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				        NULL);

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

  if(!string_empty(path))
  {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
  }

  char* filename = 0;
  if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  }

  gtk_widget_destroy(dialog);

  return filename;
}

// Display a file chooser
const char* file_dialog(GtkWidget* parent, 
						bool open, 
						const std::string& title, 
						const std::string& path, 
						const std::string& pattern)
{
  for(;;)
  {
    const char* file = file_dialog_show(
    	parent, open, title.c_str(), path.c_str(), pattern.c_str());

    if(open
      || file == 0
      || !file_exists(file)
      || gtk_MessageBox(parent, 
      					"The file specified already exists.\nDo you want to replace it?", 
      					title.c_str(), 
      					eMB_NOYES, 
      					eMB_ICONQUESTION) == eIDYES)
    {
      return file;
    }
  }
}
