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
#include "string/trim.h"
#include <wx/app.h>

namespace wxutil
{

namespace
{
	const char* const WILDCARD_EXTENSION = "*";
}

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

	// Make default extension lowercase, and cut off any dots at the beginning
	string::to_lower(_defaultExt);
	string::trim_left(_defaultExt, ".");

	if (!_open && _fileType == filetype::TYPE_MAP_EXPORT)
	{
		assembleMapExportFileTypes();
	}
	else
	{
		assembleFileTypes();
	}

	// Add a final mask for All Files (*.*)
	FileFilter wildCardFilter;

	wildCardFilter.caption = _("All Files (*.*)");
	wildCardFilter.filter = "*.*";
	wildCardFilter.extension = WILDCARD_EXTENSION;

	_fileFilters.push_back(wildCardFilter);
	
	std::string wildcard = "";

	for (const FileFilter& filter : _fileFilters)
	{
		wildcard += wildcard.empty() ? "" : "|";
		wildcard += filter.caption + "|" + filter.filter;
	}

	_dialog->SetWildcard(wildcard);

	for (int i = 0; i < _fileFilters.size(); ++i)
	{
		if (_fileFilters[i].isDefaultFilter)
		{
			_dialog->SetFilterIndex(i);
			break;
		}
	}
}

void FileChooser::assembleMapExportFileTypes()
{
	FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(_fileType);

	for (const auto& pattern : patterns)
	{
		auto formats = GlobalMapFormatManager().getMapFormatList(pattern.extension);

		// Pre-select take the default map format for this game type
		auto defaultFormat = GlobalMapFormatManager().getMapFormatForGameType(
			GlobalGameManager().currentGame()->getKeyValue("type"), pattern.extension
		);

		for (const map::MapFormatPtr& format : formats)
		{
			FileFilter filter;

			filter.caption = format->getMapFormatName() + " " + pattern.name + " (" + pattern.pattern + ")";
			filter.filter = pattern.pattern;
			filter.extension = pattern.extension;
			filter.mapFormatName = format->getMapFormatName();

			_fileFilters.push_back(filter);

			if (format == defaultFormat)
			{
				filter.isDefaultFilter = true;
			}
		}
	}
}

void FileChooser::assembleFileTypes()
{
	FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(_fileType);

	for (const auto& pattern : patterns)
	{
		FileFilter filter;

		filter.caption = pattern.name + " (" + pattern.pattern + ")";
		filter.filter = pattern.pattern;
		filter.extension = pattern.extension;

		_fileFilters.push_back(filter);

		// Pre-select the filter matching the default extension
		if (pattern.extension == _defaultExt)
		{
			filter.isDefaultFilter = true;
		}
	}
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
		selectFilterIndexFromFilename(_file);
	}
}

void FileChooser::setCurrentFile(const std::string& file)
{
	_file = os::getFilename(file);

	if (!_open)
	{
		_dialog->SetFilename(_file);
		selectFilterIndexFromFilename(_file);
	}
}

void FileChooser::selectFilterIndexFromFilename(const std::string& filename)
{
	if (filename.empty())
	{
		return;
	}

	auto ext = os::getExtension(filename);
	std::size_t wildCardIndex = std::numeric_limits<std::size_t>::max();

	for (std::size_t i = 0; i < _fileFilters.size(); ++i)
	{
		if (string::iequals(ext, _fileFilters[i].extension))
		{
			_dialog->SetFilterIndex(i);
			return;
		}

		if (_fileFilters[i].extension == WILDCARD_EXTENSION)
		{
			wildCardIndex = i;
		}
	}

	// Select the * extension if there's no better match
	if (wildCardIndex < _fileFilters.size())
	{
		_dialog->SetFilterIndex(wildCardIndex);
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
		&& os::getExtension(fileName).empty())			// no extension selected by the user
	{
		fileName.append("." + _defaultExt);
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
