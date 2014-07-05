#include "FileChooser.h"

#include "ifiletypes.h"

#include "i18n.h"
#include "imapformat.h"
#include "igame.h"
#include "os/path.h"
#include "os/file.h"

#include "MultiMonitor.h"
#include "dialog/MessageBox.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include <wx/app.h>

namespace wxutil
{

FileChooser::FileChooser(const std::string& title,
						 bool open,
						 const std::string& fileType,
						 const std::string& defaultExt) :
	_dialog(new wxFileDialog(GlobalMainFrame().getWxTopLevelWindow(), title, wxEmptyString, 
			 wxEmptyString, wxFileSelectorDefaultWildcardStr, getStyle(open))),
	_title(title),
	_fileType(fileType),
	_defaultExt(defaultExt),
	_open(open)
{
	construct();
}

FileChooser::FileChooser(wxWindow* parentWindow,
						 const std::string& title,
						 bool open,
						 const std::string& fileType,
						 const std::string& defaultExt) :
	_dialog(new wxFileDialog(parentWindow, title, wxEmptyString, 
			 wxEmptyString, wxFileSelectorDefaultWildcardStr, getStyle(open))),
	_title(title),
	_fileType(fileType),
	_defaultExt(defaultExt),
	_open(open)
{
	construct();
}

FileChooser::~FileChooser()
{
	_dialog->Destroy();

#if 0
	// Free any attached preview widgets
	if (_preview)
	{
		// Replace the existing preview with a simple label widget
		Gtk::Label* label = Gtk::manage(new Gtk::Label);
		set_preview_widget(*label);
		
		_preview.reset();
	}
#endif
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

	int defaultFormatIdx = 0;
	int curFormatIdx = 0;

	// Add the filetype
	FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(_fileType);

	for (FileTypePatterns::const_iterator i = patterns.begin(); i != patterns.end(); ++i, ++curFormatIdx)
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
				FileFilter filter;

				filter.caption = format->getMapFormatName() + " " + i->name + " (" + i->pattern + ")";
				filter.filter = i->pattern;
				filter.mapFormatName = format->getMapFormatName();

				_fileFilters.push_back(filter);

				if (format == defaultFormat)
				{
					defaultFormatIdx = curFormatIdx;
				}
			});
		}
		else
		{
			FileFilter filter;

			filter.caption = i->name + " (" + i->pattern + ")";
			filter.filter = i->pattern;

			_fileFilters.push_back(filter);
		}
	}

	// Add a final mask for All Files (*.*)
	FileFilter filter;

	filter.caption = _("All Files (*.*)");
	filter.filter = "*.*";

	_fileFilters.push_back(filter);
	
	std::string wildcard = "";

	std::for_each(_fileFilters.begin(), _fileFilters.end(), [&] (const FileFilter& filter)
	{
		wildcard += wildcard.empty() ? "" : "|";
		wildcard += filter.caption + "|" + filter.filter;
	});

	_dialog->SetWildcard(wildcard);
	_dialog->SetFilterIndex(defaultFormatIdx);
}

long FileChooser::getStyle(bool open)
{
	if (open)
	{
		return wxFD_OPEN;
	}
	else // !open
	{
		return wxFD_SAVE | wxFD_OVERWRITE_PROMPT;
	}
}

void FileChooser::setCurrentPath(const std::string& path)
{
	_path = os::standardPathWithSlash(path);

	// Convert path to standard and set the folder in the dialog
	_dialog->SetPath(_path);

	// SetPath() overwrites the filename, so set it again
	if (!_file.empty())
	{
		_dialog->SetFilename(_file);
	}
}

void FileChooser::setCurrentFile(const std::string& file)
{
	_file = os::getFilename(file);

	if (!_open)
	{
		_dialog->SetFilename(_file);
	}
}

std::string FileChooser::getSelectedFileName()
{
	// Load the filename from the dialog
	std::string fileName = os::standardPath(_dialog->GetPath().ToStdString());

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
	int index = _dialog->GetFilterIndex();

	if (index >=0 && index < _fileFilters.size())
	{
		return _fileFilters[index].mapFormatName;
	}
	
	return "";
}

void FileChooser::askForOverwrite(bool ask)
{
	if (ask)
	{
		_dialog->SetWindowStyleFlag(_dialog->GetWindowStyleFlag() | wxFD_OVERWRITE_PROMPT);
	}
	else
	{
		_dialog->SetWindowStyleFlag(_dialog->GetWindowStyleFlag() & ~wxFD_OVERWRITE_PROMPT);
	}
}

void FileChooser::attachPreview(const PreviewPtr& preview)
{
#if 0
	if (_preview != NULL)
	{
		rError() << "Error, preview already attached to FileChooser." << std::endl;
		return;
	}

	_preview = preview;

	set_preview_widget(_preview->getPreviewWidget());

	signal_update_preview().connect(sigc::mem_fun(*this, &FileChooser::onUpdatePreview));
#endif
}

std::string FileChooser::display()
{
	int curDisplayIdx = wxDisplay::GetFromWindow(wxTheApp->GetTopWindow());
	wxDisplay curDisplay(curDisplayIdx);

	wxRect rect = curDisplay.GetGeometry();
	int newWidth = static_cast<int>(rect.GetWidth() * 0.5f);
	int newHeight = static_cast<int>(rect.GetHeight() * 0.66f);

	_dialog->SetSize(newWidth, newHeight);
	_dialog->CenterOnScreen();

	if (_dialog->ShowModal() == wxID_OK)
	{
		return getSelectedFileName();
	}

	return "";

#if 0
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

			wxutil::Messagebox box(askTitle, askMsg, ui::IDialog::MESSAGE_ASK);

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
#endif
}

void FileChooser::setPreviewActive(bool active)
{
#if 0
	// Pass the call to the dialog
	set_preview_widget_active(active);
#endif
}

void FileChooser::onUpdatePreview()
{
#if 0
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
#endif
}

} // namespace wxutil

#if 0
#include <gtkmm/stock.h>
#include <gtkmm/label.h>

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

			wxutil::Messagebox box(askTitle, askMsg, ui::IDialog::MESSAGE_ASK);

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
#endif
