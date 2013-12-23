#include "FileChooser.h"

#include "ifiletypes.h"

#include <gtkmm/stock.h>
#include <gtkmm/label.h>

#include "i18n.h"
#include "imapformat.h"
#include "igame.h"
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
						 const std::string& fileType,
						 const std::string& defaultExt) :
	Gtk::FileChooserDialog(title, getActionType(browseFolders, open)),
	_title(title),
	_fileType(fileType),
	_defaultExt(defaultExt),
	_open(open),
	_browseFolders(browseFolders),
	_askOverwrite(true)
{
	construct();
}

FileChooser::FileChooser(const Glib::RefPtr<Gtk::Window>& parentWindow,
						 const std::string& title,
						 bool open,
						 bool browseFolders,
						 const std::string& fileType,
						 const std::string& defaultExt) :
	Gtk::FileChooserDialog(title, getActionType(browseFolders, open)),
	_parent(parentWindow),
	_title(title),
	_fileType(fileType),
	_defaultExt(defaultExt),
	_open(open),
	_browseFolders(browseFolders),
	_askOverwrite(true)
{
	construct();
}

FileChooser::~FileChooser()
{
	hide();

	// Free any attached preview widgets
	if (_preview)
	{
		// Replace the existing preview with a simple label widget
		Gtk::Label* label = Gtk::manage(new Gtk::Label);
		set_preview_widget(*label);
		
		_preview.reset();
	}
}

void FileChooser::construct()
{
	// Sanity-check the filetype
	if (_fileType.empty())
	{
		_fileType = "*";
	}

	// Set a meaningful title if empty
	if (_title.empty())
	{
		_title = _open ? _("Open File") : _("Save File");
	}

	if (_parent)
	{
		set_parent_window(_parent->get_window());
	}

	set_position(Gtk::WIN_POS_CENTER);

	// Add cancel button
	add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

	// Add OK button
	if (_open)
	{
		add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
	}
	else
	{
		add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	}

	// Set the Enter key to activate the default response
	set_default_response(Gtk::RESPONSE_ACCEPT);

	// Set position and modality of the dialog
	set_modal(true);

	// Set the default size of the window
	Gdk::Rectangle rect = _parent 
                           ? MultiMonitor::getMonitorForWindow(_parent)
                           : MultiMonitor::getMonitor(0);

	set_default_size(static_cast<int>(rect.get_width()/2), static_cast<int>(2*rect.get_height()/3));

	// Add the filetype
	FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(_fileType);

	for (FileTypePatterns::const_iterator i = patterns.begin(); i != patterns.end(); ++i)
	{
		if (!_open && _fileType == "map")
		{
			std::set<map::MapFormatPtr> formats = GlobalMapFormatManager().getMapFormatList(i->extension);

			// Pre-select take the default map format for this game type
			map::MapFormatPtr defaultFormat = GlobalMapFormatManager().getMapFormatForGameType(
				GlobalGameManager().currentGame()->getKeyValue("type"), i->extension
			);

			std::for_each(formats.begin(), formats.end(), [&] (const map::MapFormatPtr& format)
			{
				// Create a GTK file filter and add it to the chooser dialog
				Gtk::FileFilter* filter = Gtk::manage(new Gtk::FileFilter);
				filter->add_pattern(i->pattern);
				filter->set_name(format->getMapFormatName() + " " + i->name + " (" + i->pattern + ")");
				filter->set_data("format", const_cast<void*>(static_cast<const void*>(format->getMapFormatName().c_str())));

				add_filter(*filter);

				if (format == defaultFormat)
				{
					set_filter(*filter);
				}
			});
		}
		else
		{
			// Create a GTK file filter and add it to the chooser dialog
			Gtk::FileFilter* filter = Gtk::manage(new Gtk::FileFilter);
			filter->add_pattern(i->pattern);
			filter->set_name(i->name + " (" + i->pattern + ")");
			add_filter(*filter);
		}
	}

	// Add a final mask for All Files (*.*)
	Gtk::FileFilter* allFilter = Gtk::manage(new Gtk::FileFilter);

	allFilter->add_pattern("*.*");
	allFilter->set_name(_("All Files (*.*)"));

	add_filter(*allFilter);
}

Gtk::FileChooserAction FileChooser::getActionType(bool browseFolders, bool open)
{
	if (browseFolders)
	{
		return Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER;
	}
	else if (open)
	{
		return Gtk::FILE_CHOOSER_ACTION_OPEN;
	}
	else // !open
	{
		return Gtk::FILE_CHOOSER_ACTION_SAVE;
	}
}

void FileChooser::setCurrentPath(const std::string& path)
{
	_path = os::standardPath(path);

	// Convert path to standard and set the folder in the dialog
	set_current_folder(_path);
}

void FileChooser::setCurrentFile(const std::string& file)
{
	_file = file;

	if (!_open)
	{
		set_current_name(_file);
	}
}

std::string FileChooser::getSelectedFileName()
{
	// Load the filename from the dialog
	std::string fileName = os::standardPath(get_filename());

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

std::string FileChooser::getSelectedMapFormat()
{
	Gtk::FileFilter* filter = get_filter();

	if (filter != NULL)
	{
		void* data = filter->get_data("format");
		return data != NULL ? std::string(static_cast<char*>(data)) : "";
	}
	else
	{
		return "";
	}
}

void FileChooser::askForOverwrite(bool ask)
{
	_askOverwrite = ask;
}

void FileChooser::attachPreview(const PreviewPtr& preview)
{
	if (_preview != NULL)
	{
		rError() << "Error, preview already attached to FileChooser." << std::endl;
		return;
	}

	_preview = preview;

	set_preview_widget(_preview->getPreviewWidget());

	signal_update_preview().connect(sigc::mem_fun(*this, &FileChooser::onUpdatePreview));
}

std::string FileChooser::display()
{
	// Loop until break
	while (1)
	{
		// Display the dialog and return the selected filename, or ""
		std::string fileName("");

		if (run() == Gtk::RESPONSE_ACCEPT)
		{
			// "OK" pressed, retrieve the filename
			fileName = getSelectedFileName();
		}

		// Always return the fileName for "open" and empty filenames, otherwise check file existence
		if (_open || fileName.empty())
		{
			return fileName;
		}

		if (!os::fileOrDirExists(fileName))
		{
			return fileName;
		}

		if (_askOverwrite)
		{
			// If file exists, ask for overwrite
			std::string askTitle = _title;
			askTitle += (!fileName.empty()) ? ": " + os::getFilename(fileName) : "";

			std::string askMsg = (boost::format("The file %s already exists.") % os::getFilename(fileName)).str();
			askMsg += "\n";
			askMsg += _("Do you want to replace it?");

			MessageBox box(askTitle, askMsg, ui::IDialog::MESSAGE_ASK);

			if (box.run() == ui::IDialog::RESULT_YES)
			{
				return fileName;
			}
		}
		else
		{
			// Don't ask questions, return the selected filename
			return fileName;
		}
	}
}

void FileChooser::setPreviewActive(bool active)
{
	// Pass the call to the dialog
	set_preview_widget_active(active);
}

void FileChooser::onUpdatePreview()
{
	// Check if we have a valid preview object attached
	if (_preview == NULL) return;

	std::string previewFilename = get_preview_filename();

	previewFilename = os::standardPath(previewFilename);

	// Check the file type
	Glib::RefPtr<Gio::File> file = get_preview_file();

	if (file && file->query_file_type() == Gio::FILE_TYPE_DIRECTORY)
	{
		return; // don't preview folders
	}

	// Emit the signal
	_preview->onFileSelectionChanged(previewFilename, *this);
}

} // namespace gtkutil
