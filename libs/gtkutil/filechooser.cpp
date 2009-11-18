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
#include <gtk/gtkimage.h>

#include "os/path.h"
#include "os/file.h"

#include "MultiMonitor.h"
#include "messagebox.h"
#include <boost/algorithm/string/predicate.hpp>

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

namespace gtkutil {

FileChooser::FileChooser(GtkWidget* parent, const std::string& title, 
						 bool open, bool browseFolders, const std::string& pattern,
						 const std::string& defaultExt) :
	_parent(parent),
	_dialog(NULL),
	_title(title),
	_pattern(pattern),
	_defaultExt(defaultExt),
	_open(open)
{
	if (_pattern.empty()) {
		_pattern = "*";
	}

	if (_title.empty()) {
		_title = _open ? "Open File" : "Save File";
	}

	if (_open) {
		_dialog = gtk_file_chooser_dialog_new(_title.c_str(),
			GTK_WINDOW(_parent),
			browseFolders ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	}
	else {
		_dialog = gtk_file_chooser_dialog_new(title.c_str(),
			GTK_WINDOW(_parent),
			browseFolders ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
			NULL);
	}

	// Set the Enter key to activate the default response
	gtk_dialog_set_default_response(GTK_DIALOG(_dialog), GTK_RESPONSE_ACCEPT);

	// Set position and modality of the dialog
	gtk_window_set_modal(GTK_WINDOW(_dialog), TRUE);
	gtk_window_set_position(GTK_WINDOW(_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	// Set the default size of the window
	GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GTK_WINDOW(_parent));

	gtk_window_set_default_size(
		GTK_WINDOW(_dialog), gint(rect.width/2), gint(2*rect.height/3)
	);

	// Add the filetype masks
	ModuleTypeListPtr typeList = GlobalFiletypes().getTypesFor(_pattern);
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
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(_dialog), filter);
	}

	// Add a final mask for All Files (*.*)
	GtkFileFilter* allFilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(allFilter, "*.*");
	gtk_file_filter_set_name(allFilter, "All Files (*.*)");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(_dialog), allFilter);
}

FileChooser::~FileChooser() {
	// Destroy the dialog
	gtk_widget_destroy(_dialog);
}

void FileChooser::setCurrentPath(const std::string& path) {
	_path = os::standardPath(path);

	// Convert path to standard and set the folder in the dialog
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(_dialog), _path.c_str());
}

void FileChooser::setCurrentFile(const std::string& file) {
	_file = file;

	if (!_open) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(_dialog), _file.c_str());
	}
}

std::string FileChooser::getSelectedFileName() {
	// Load the filename from the dialog
	std::string fileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(_dialog));

	fileName = os::standardPath(fileName);

	// Append the default extension for save operations before checking overwrites
	if (!_open											// save operation
	    && !fileName.empty() 							// valid filename
	    && !_defaultExt.empty()							// non-empty default extension
	    && !boost::algorithm::iends_with(fileName, _defaultExt)) // no default extension
	{
		fileName.append(_defaultExt);
	}

	return fileName;
}

void FileChooser::attachPreview(const PreviewPtr& preview) {
	if (_preview != NULL) {
		globalErrorStream() << "Error, preview already attached to FileChooser." << std::endl;
		return;
	}

	_preview = preview;

	gtk_file_chooser_set_preview_widget(
		GTK_FILE_CHOOSER(_dialog), 
		_preview->getPreviewWidget()
	);

	g_signal_connect(G_OBJECT(_dialog), "update-preview", G_CALLBACK(onUpdatePreview), this);
}

std::string FileChooser::display() {
	// Loop until break
	while (1) {
		// Display the dialog and return the selected filename, or ""
		std::string fileName("");

		if (gtk_dialog_run(GTK_DIALOG(_dialog)) == GTK_RESPONSE_ACCEPT) {
			// "OK" pressed, retrieve the filename
			fileName = getSelectedFileName();
		}

		// Always return the fileName for "open" and empty filenames, otherwise check file existence 
		if (_open || fileName.empty()) {
			return fileName;
		}

		// If file exists, ask for overwrite
		std::string askTitle = _title;
		askTitle += (!fileName.empty()) ? ": " + os::getFilename(fileName) : "";

		if (!file_exists(fileName.c_str()) || 
			 gtk_MessageBox(_parent,
		                    "The specified file already exists.\nDo you want to replace it?",
		                    askTitle.c_str(),
		                    eMB_NOYES,
		                    eMB_ICONQUESTION) == eIDYES)
		{
			return fileName;
		}
	}
}

void FileChooser::setPreviewActive(bool active) {
	// Pass the call to the dialog
	gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(_dialog), active ? TRUE : FALSE);
}

void FileChooser::onUpdatePreview(GtkFileChooser* chooser, FileChooser* self) {
	// Check if we have a valid preview object attached
	if (self->_preview == NULL) return;

	char* sel = gtk_file_chooser_get_preview_filename(GTK_FILE_CHOOSER(self->_dialog));

	std::string previewFileName = (sel != NULL) ? sel : "";

	g_free(sel);

	previewFileName = os::standardPath(previewFileName);

	// Emit the signal
	self->_preview->onFileSelectionChanged(previewFileName, *self);
}

} // namespace gtkutil

