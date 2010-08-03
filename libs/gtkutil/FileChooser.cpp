#include "FileChooser.h"

#include "ifiletypes.h"

#include <gtkmm/stock.h>

#include "i18n.h"
#include "os/path.h"
#include "os/file.h"

#include "MultiMonitor.h"
#include "dialog/MessageBox.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>

namespace gtkutil
{

FileChooser::FileChooser(const std::string& title, 
						 bool open, 
						 bool browseFolders, 
						 const std::string& pattern,
						 const std::string& defaultExt) :
	_title(title),
	_pattern(pattern),
	_defaultExt(defaultExt),
	_open(open),
	_browseFolders(browseFolders)
{
	construct();
}

FileChooser::FileChooser(const Glib::RefPtr<Gtk::Window>& parentWindow, 
						 const std::string& title, 
						 bool open, 
						 bool browseFolders, 
						 const std::string& pattern,
						 const std::string& defaultExt) :
	_parent(parentWindow),
	_title(title),
	_pattern(pattern),
	_defaultExt(defaultExt),
	_open(open),
	_browseFolders(browseFolders)
{
	construct();
}

void FileChooser::construct()
{
	// Sanity-check the pattern
	if (_pattern.empty())
	{
		_pattern = "*";
	}

	// Set a meaningful title if empty
	if (_title.empty())
	{
		_title = _open ? _("Open File") : _("Save File");
	}

	// Determine action
	Gtk::FileChooserAction action;

	if (_browseFolders)
	{
		action = Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER;
	}
	else if (_open)
	{
		action = Gtk::FILE_CHOOSER_ACTION_OPEN;
	}
	else // !open
	{
		action = Gtk::FILE_CHOOSER_ACTION_SAVE;
	}

	_dialog = Glib::RefPtr<Gtk::FileChooserDialog>(new Gtk::FileChooserDialog(_title, action));

	if (_parent)
	{
		_dialog->set_parent_window(_parent->get_window());
	}

	// Add cancel button
	_dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

	// Add OK button
	if (_open)
	{
		_dialog->add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
	}
	else
	{
		_dialog->add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	}

	// Set the Enter key to activate the default response
	_dialog->set_default_response(Gtk::RESPONSE_ACCEPT);
	
	// Set position and modality of the dialog
	_dialog->set_modal(true);
	_dialog->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	// Set the default size of the window
	Gdk::Rectangle rect = _parent != NULL ? MultiMonitor::getMonitorForWindow(_parent) : MultiMonitor::getMonitor(0);

	_dialog->set_default_size(static_cast<int>(rect.get_width()/2), static_cast<int>(2*rect.get_height()/3));
	
	// Add the filetype masks
	ModuleTypeListPtr typeList = GlobalFiletypes().getTypesFor(_pattern);

	for (ModuleTypeList::iterator i = typeList->begin();
	 	 i != typeList->end();
		 ++i)
	{
		// Create a GTK file filter and add it to the chooser dialog
		Gtk::FileFilter* filter = Gtk::manage(new Gtk::FileFilter);
		filter->add_pattern(i->filePattern.pattern);
		filter->set_name(i->filePattern.name + " (" + i->filePattern.pattern + ")");

		_dialog->add_filter(*filter);
	}

	// Add a final mask for All Files (*.*)
	Gtk::FileFilter* allFilter = Gtk::manage(new Gtk::FileFilter);

	allFilter->add_pattern("*.*");
	allFilter->set_name(_("All Files (*.*)"));

	_dialog->add_filter(*allFilter);
}

void FileChooser::setCurrentPath(const std::string& path)
{
	_path = os::standardPath(path);

	// Convert path to standard and set the folder in the dialog
	_dialog->set_current_folder(_path);
}

void FileChooser::setCurrentFile(const std::string& file)
{
	_file = file;

	if (!_open)
	{
		_dialog->set_current_name(_file);
	}
}

std::string FileChooser::getSelectedFileName()
{
	// Load the filename from the dialog
	std::string fileName = os::standardPath(_dialog->get_filename());

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

void FileChooser::attachPreview(const PreviewPtr& preview)
{
	if (_preview != NULL)
	{
		globalErrorStream() << "Error, preview already attached to FileChooser." << std::endl;
		return;
	}

	_preview = preview;

	_dialog->set_preview_widget(_preview->getPreviewWidget());

	_dialog->signal_update_preview().connect(sigc::mem_fun(*this, &FileChooser::onUpdatePreview));
}

std::string FileChooser::display()
{
	// Loop until break
	while (1)
	{
		// Display the dialog and return the selected filename, or ""
		std::string fileName("");

		if (_dialog->run() == GTK_RESPONSE_ACCEPT)
		{
			// "OK" pressed, retrieve the filename
			fileName = getSelectedFileName();
		}

		// Always return the fileName for "open" and empty filenames, otherwise check file existence 
		if (_open || fileName.empty())
		{
			return fileName;
		}

		if (!file_exists(fileName.c_str()))
		{
			return fileName;
		}

		// If file exists, ask for overwrite
		std::string askTitle = _title;
		askTitle += (!fileName.empty()) ? ": " + os::getFilename(fileName) : "";

		std::string askMsg = (boost::format("The file %s already exists.") % os::getFilename(fileName)).str();
		askMsg += "\n";
		askMsg += _("Do you want to replace it?");

		MessageBox box(askTitle, askMsg, ui::IDialog::MESSAGE_ASK, _dialog);

		if (box.run() == ui::IDialog::RESULT_YES)
		{
			return fileName;
		}
	}
}

void FileChooser::setPreviewActive(bool active)
{
	// Pass the call to the dialog
	_dialog->set_preview_widget_active(active);
}

void FileChooser::onUpdatePreview()
{
	// Check if we have a valid preview object attached
	if (_preview == NULL) return;

	std::string previewFilename = _dialog->get_preview_filename();

	previewFilename = os::standardPath(previewFilename);

	// Emit the signal
	_preview->onFileSelectionChanged(previewFilename, *this);
}

} // namespace gtkutil
