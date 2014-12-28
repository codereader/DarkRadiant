#pragma once

#include "idialogmanager.h"
#include "imainframe.h"
#include <wx/msgdlg.h>
#include <wx/frame.h>

namespace wxutil
{

/**
 * A MessageBox is a specialised Dialog used for popup messages of various purpose.
 * Supported are things like Notifications, Warnings, Errors and simple Yes/No questions.
 *
 * Each messagebox is equipped with a special stock icon, corresponding to its type.
 *
 * Note: had to change this to lowercase to not conflict with the MessageBox #define in 
 * some of those windows headers.
 */
class Messagebox :
	public ui::IDialog
{
protected:
	wxMessageDialog* _dialog;

	// The message text
	std::string _text;

	// The message type
	ui::IDialog::MessageType _type;

public:
	// Constructs a new messageBox using the given title and text
	Messagebox(const std::string& title,
			   const std::string& text,
			   ui::IDialog::MessageType type,
			   wxWindow* parent = NULL);

protected:
	// Used during construction
	long getDialogStyle(ui::IDialog::MessageType type);

public:
	virtual ~Messagebox();

	virtual void setTitle(const std::string& title);

	// We implement these using an empty body
	virtual Handle addLabel(const std::string& text) { return ui::INVALID_HANDLE; }
	virtual Handle addComboBox(const std::string& label, const ComboBoxOptions& options) { return ui::INVALID_HANDLE; }
	virtual Handle addEntryBox(const std::string& label) { return ui::INVALID_HANDLE; }
	virtual Handle addPathEntry(const std::string& label, bool foldersOnly = false) { return ui::INVALID_HANDLE; }
	virtual Handle addSpinButton(const std::string& label, double min, double max, double step, unsigned int digits) { return ui::INVALID_HANDLE; }
	virtual Handle addCheckbox(const std::string& label) { return ui::INVALID_HANDLE; }

	virtual void setElementValue(const Handle& handle, const std::string& value) {}
	virtual std::string getElementValue(const Handle& handle) { return ""; }

	// Enter the main loop
	virtual ui::IDialog::Result run();

	// Static methods

	/**
	 * Display a message box of the given type, using title as window caption
	 * and text as content to display. Will return the result code.
	 */
	static ui::IDialog::Result Show(const std::string& title,
			   const std::string& text,
			   ui::IDialog::MessageType type,
			   wxWindow* parent = GlobalMainFrame().getWxTopLevelWindow());

	/**
	 * Display a modal error dialog. 
	 */
	static void ShowError(const std::string& errorText, 
						  wxWindow* parent = GlobalMainFrame().getWxTopLevelWindow());

	/**
	 * Display a modal error dialog and quit immediately.
	 */
	static void ShowFatalError(const std::string& errorText, 
							   wxWindow* parent = GlobalMainFrame().getWxTopLevelWindow());
};
typedef std::shared_ptr<Messagebox> MessageboxPtr;

} // namespace
