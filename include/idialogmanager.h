#ifndef _IDIALOG_MANAGER_H_
#define _IDIALOG_MANAGER_H_

#include "iuimanager.h"
#include <boost/shared_ptr.hpp>
#include <gtkmm/window.h>

namespace ui
{

class IDialog
{
public:
	virtual ~IDialog() {}

	enum Result
	{
		RESULT_CANCELLED = 0,
		RESULT_OK,
		RESULT_NO,
		RESULT_YES,
	};

	/// Possible message types (used for IDialogManager::createMessageBox())
	enum MessageType
	{
        /// Just a plain message with an OK button
		MESSAGE_CONFIRM,

        // Queries Yes//No from the user
		MESSAGE_ASK,

        /// Displays a warning message
		MESSAGE_WARNING,

        /// Displays an error message
		MESSAGE_ERROR,

        /// Has three options: Yes, No or Cancel
		MESSAGE_YESNOCANCEL,

        /// Save confirmation as per HIG 2.32/3.4.6.1
        MESSAGE_SAVECONFIRMATION
	};

	// Sets the dialog title
	virtual void setTitle(const std::string& title) = 0;

	// A handle to access dialog elements after addition
	typedef std::size_t Handle;
	typedef std::vector<std::string> ComboBoxOptions;

	// ------------------- Elements -----------------------

	/**
	 * Adds a simple label at the current position in the dialog.
	 * A unique handle is returned to allow for later value retrieval.
	 * The elements are inserted in the order of calls, top to bottom.
	 * In case of errors an invalid handle (==0) is returned.
	 */
	virtual Handle addLabel(const std::string& text) = 0;
	virtual Handle addComboBox(const std::string& label, const ComboBoxOptions& options) = 0;
	virtual Handle addEntryBox(const std::string& label) = 0;
	virtual Handle addPathEntry(const std::string& label, bool foldersOnly = false) = 0;
	virtual Handle addSpinButton(const std::string& label, double min, double max, double step, unsigned int digits) = 0;
	virtual Handle addCheckbox(const std::string& label) = 0;

	// ----------------- Element Value --------------------

	// Retrieve or set an element's value by string
	virtual void setElementValue(const Handle& handle, const std::string& value) = 0;
	virtual std::string getElementValue(const Handle& handle) = 0;

	// ----------------------------------------------------

	/**
	 * Run the dialog an enter the main loop (block the application).
	 * Returns the Dialog::Result, corresponding to the user's action.
	 */
	virtual Result run() = 0;
};
typedef boost::shared_ptr<IDialog> IDialogPtr;

const IDialog::Handle INVALID_HANDLE = 0;

class IFileChooser;
typedef boost::shared_ptr<IFileChooser> IFileChooserPtr;

class IDialogManager
{
public:
	// Virtual destructor
	virtual ~IDialogManager() {}

	/**
	 * Create a new dialog. Note that the DialogManager will hold a reference
	 * to this dialog internally to allow scripts to reference the Dialog class
	 * without holding the shared_ptr on their own or using wrapper classes doing so.
	 *
	 * Every dialog features an OK and a Cancel button by default.
	 *
	 * @title: The string displayed on the dialog's window bar.
	 * @type: the dialog type to create, determines e.g. which buttons are shown.
	 * @parent: optional top-level widget this dialog should be parented to, defaults to
	 *			GlobalMainFrame().getMainWindow().
	 */
	virtual IDialogPtr createDialog(const std::string& title,
									const Glib::RefPtr<Gtk::Window>& parent = Glib::RefPtr<Gtk::Window>()) = 0;

	/**
	 * Create a simple message box, which can either notify the user about something,
	 * queries "Yes"/"No" or displays an error message. It usually features
	 * an icon according to the the MessageType passed (exclamation mark, error sign).
	 *
	 * @title: The string displayed on the message box window bar.
	 * @text: The text/question to be displayed.
	 * @type: the message type this dialog represents.
	 * @parent: optional top-level widget this dialog should be parented to, defaults to
	 *			GlobalMainFrame().getMainWindow().
	 */
	virtual IDialogPtr createMessageBox(const std::string& title,
                                        const std::string& text,
										IDialog::MessageType type,
										const Glib::RefPtr<Gtk::Window>& parent = Glib::RefPtr<Gtk::Window>()) = 0;

	/**
	 * Acquire a new filechooser instance with the given parameters.
	 *
	 * @title: The dialog title.
	 * @open: if TRUE this is asking for "Open" files, FALSE generates a "Save" dialog.
	 * @browseFolders: if TRUE this is asking for folders, not files.
	 * @pattern: the type "map", "prefab", this determines the file extensions.
	 * @defaultExt: The default extension appended when the user enters
	 *              filenames without extension.
 	 */
	virtual ui::IFileChooserPtr createFileChooser(const std::string& title,
												bool open, bool browseFolders,
												const std::string& pattern = "",
												const std::string& defaultExt = "") = 0;
};

} // namespace ui

// Shortcut method
inline ui::IDialogManager& GlobalDialogManager()
{
	return GlobalUIManager().getDialogManager();
}

#endif /* _IDIALOG_MANAGER_H_ */
