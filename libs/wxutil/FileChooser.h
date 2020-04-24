#pragma once

#include "ifilechooser.h"
#include <string>
#include <memory>
#include <wx/filedlg.h>
#include <vector>

namespace wxutil
{

class FileChooser :
	public ui::IFileChooser
{
private:
	wxFileDialog* _dialog;

	// Window title
	std::string _title;

	std::string _path;
	std::string _file;

	std::string _fileType;

	std::string _defaultExt;

	// Open or save dialog
	bool _open;

	struct FileFilter
	{
		std::string caption;	// "Doom 3 Map (*.map)"
		std::string filter;		// "*.map"
		std::string extension;		// "map"
		std::string mapFormatName;
		bool isDefaultFilter;			// should be selected when dialog is shown

		FileFilter() :
			isDefaultFilter(false)
		{}
	};

	std::vector<FileFilter> _fileFilters;

public:
	/**
	 * Construct a new filechooser with the given parameters.
	 *
	 * @title: The dialog title.
	 * @open: if TRUE this is asking for "Open" files, FALSE generates a "Save" dialog.
	 * @browseFolders: if TRUE the dialog is asking the user for directories only.
	 * @fileType: the type "map", "prefab", this determines the file extensions.
	 * @defaultExt: The default extension appended when the user enters
	 *              filenames without extension.
 	 */
	FileChooser(const std::string& title,
				bool open,
				const std::string& fileType = "",
				const std::string& defaultExt = "");

	/**
	 * Construct a new filechooser with the given parameters.
	 *
	 * @parentWindow: The parent window, must not be NULL.
	 * @title: The dialog title.
	 * @open: if TRUE this is asking for "Open" files, FALSE generates a "Save" dialog.
	 * @browseFolders: if TRUE the dialog is asking the user for directories only.
	 * @pattern: the type "map", "prefab", this determines the file extensions.
	 * @defaultExt: The default extension appended when the user enters
	 *              filenames without extension.
 	 */
	FileChooser(wxWindow* parentWindow,
				const std::string& title,
				bool open,
				const std::string& fileType = "",
				const std::string& defaultExt = "");

	virtual ~FileChooser();

	// Lets the dialog start at a certain path
	void setCurrentPath(const std::string& path);

	// Pre-fills the currently selected file
	void setCurrentFile(const std::string& file);

	/**
	 * Returns the selected filename (default extension
	 * will be added if appropriate).
	 */
	virtual std::string getSelectedFileName();

	/**
	 * Get the selected map format by name (corresponds to the
	 * string as returned by MapFormat::getMapFormatName()).
	 */
	virtual std::string getSelectedMapFormat();

	/**
	 * It's possible to inihibit the "File exists - replace" question when
	 * selecting filenames for saving.
	 */
	void askForOverwrite(bool ask);

	/**
	 * greebo: Displays the dialog and enters a blocking loop.
	 * Returns the filename or "" if the user hit cancel.
	 *
	 * The returned file name is normalised using the os::standardPath() method.
	 */
	virtual std::string display();

private:
	static long getStyle(bool open);

	void selectFilterIndexFromFilename(const std::string& filename);

	void construct(); // shared constructor stuff

	void assembleMapExportFileTypes();
	void assembleFileTypes();
};

} // namespace wxutil
