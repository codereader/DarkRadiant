#include "FileChooser.h"

#include "ifiletypes.h"

#include "i18n.h"
#include "imapformat.h"
#include "igame.h"
#include "os/path.h"
#include "os/file.h"

#include "MultiMonitor.h"
#include "dialog/MessageBox.h"
#include "string/predicate.h"
#include "string/case_conv.h"
#include <wx/app.h>

namespace wxutil
{

FileChooser::FileChooser(const std::string& title,
						 bool open,
						 const std::string& fileType,
						 const std::string& defaultExt) :
	FileChooser(GlobalMainFrame().getWxTopLevelWindow(), title, open, fileType, defaultExt)
{}

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

	// Make default extension lowercase
	string::to_lower(_defaultExt);

	int defaultFormatIdx = 0;
	int curFormatIdx = 0;

	// Add the filetype
	FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(_fileType);

	for (FileTypePatterns::const_iterator i = patterns.begin(); i != patterns.end(); ++i)
	{
		if (!_open && _fileType == filetype::TYPE_MAP)
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

                ++curFormatIdx;
			});
		}
		else
		{
			FileFilter filter;

			filter.caption = i->name + " (" + i->pattern + ")";
			filter.filter = i->pattern;

			// Pre-select the filter matching the default extension
			if (i->extension == _defaultExt)
			{
				defaultFormatIdx = curFormatIdx;
			}

			_fileFilters.push_back(filter);

			++curFormatIdx;
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
	    && !string::iends_with(fileName, _defaultExt)) // no default extension
	{
		fileName.append(_defaultExt);
	}

	return fileName;
}

std::string FileChooser::getSelectedMapFormat()
{
	int index = _dialog->GetFilterIndex();

	if (index >=0 && index < static_cast<int>(_fileFilters.size()))
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
}

} // namespace wxutil
