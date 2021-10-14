#pragma once

#include <memory>
#include <string>

class wxWindow;

namespace ui
{

/**
 * The FileChooser class can be used to query a path from the user.
 * Use the GlobalDialogManager module to acquire a new instance of this class.
 */
class IFileChooser
{
public:
	virtual ~IFileChooser() {}

	// Lets the dialog start at a certain path
	virtual void setCurrentPath(const std::string& path) = 0;

	// Pre-fills the currently selected file
	virtual void setCurrentFile(const std::string& file) = 0;

	/**
	 * Returns the selected filename (default extension
	 * will be added if appropriate).
	 */
	virtual std::string getSelectedFileName() = 0;

	/**
	 * greebo: Displays the dialog and enters the main loop.
	 * Returns the filename or "" if the user hit cancel.
	 *
	 * The returned file name is normalised using the os::standardPath() method.
	 */
	virtual std::string display() = 0;
};
typedef std::shared_ptr<IFileChooser> IFileChooserPtr;

/**
 * The DirChooser class can be used to query a directory from the user.
 * Use the GlobalDialogManager module to acquire a new instance of this class.
 */
class IDirChooser
{
public:
	virtual ~IDirChooser() {}

	// Lets the dialog start at a certain path
	virtual void setCurrentPath(const std::string& path) = 0;

	/**
	 * Returns the selected folder name.
	 */
	virtual std::string getSelectedFolderName() = 0;

	/**
	 * greebo: Displays the dialog and enters the main loop.
	 * Returns the filename or "" if the user hit cancel.
	 *
	 * The returned file name is normalised using the os::standardPath() method.
	 */
	virtual std::string display() = 0;
};
typedef std::shared_ptr<IDirChooser> IDirChooserPtr;

} // namespace ui

