#ifndef _SCRIPT_DIALOG_INTERFACE_H_
#define _SCRIPT_DIALOG_INTERFACE_H_

#include <boost/python.hpp>

#include "iscript.h"
#include "idialogmanager.h"

namespace script
{

// Wrapper class around IDialog
class ScriptDialog
{
	ui::IDialogPtr _dialog;
public:
	ScriptDialog(const ui::IDialogPtr& dialog) :
		_dialog(dialog)
	{}

	void setTitle(const std::string& title)
	{
		if (_dialog != NULL) _dialog->setTitle(title);
	}

	ui::IDialog::Result run()
	{
		return (_dialog != NULL) ? _dialog->run() : ui::IDialog::RESULT_CANCELLED;
	}

	ui::IDialog::Handle addLabel(const std::string& text)
	{
		return (_dialog != NULL) ? _dialog->addLabel(text) : ui::INVALID_HANDLE;
	}

	ui::IDialog::Handle addComboBox(const std::string& label, const ui::IDialog::ComboBoxOptions& options)
	{
		return (_dialog != NULL) ? _dialog->addComboBox(label, options) : ui::INVALID_HANDLE;
	}

	ui::IDialog::Handle addEntryBox(const std::string& label)
	{
		return (_dialog != NULL) ? _dialog->addEntryBox(label) : ui::INVALID_HANDLE;
	}

	ui::IDialog::Handle addPathEntry(const std::string& label, bool foldersOnly = false)
	{
		return (_dialog != NULL) ? _dialog->addPathEntry(label, foldersOnly) : ui::INVALID_HANDLE;
	}

	ui::IDialog::Handle addSpinButton(const std::string& label, double min, double max, double step, unsigned int digits)
	{
		return (_dialog != NULL) ? _dialog->addSpinButton(label, min, max, step, digits) : ui::INVALID_HANDLE;
	}

	ui::IDialog::Handle addCheckbox(const std::string& label)
	{
		return (_dialog != NULL) ? _dialog->addCheckbox(label) : ui::INVALID_HANDLE;
	}

	void setElementValue(const ui::IDialog::Handle& handle, const std::string& value)
	{
		if (_dialog != NULL) _dialog->setElementValue(handle, value);
	}

	std::string getElementValue(const ui::IDialog::Handle& handle)
	{
		return (_dialog != NULL) ? _dialog->getElementValue(handle) : "";
	}
};

/**
 * greebo: This class provides the script interface for the DialogManager class (UIManager module).
 */
class DialogManagerInterface :
	public IScriptInterface
{
public:
	ScriptDialog createDialog(const std::string& title);
	ScriptDialog createMessageBox(const std::string& title, const std::string& text, ui::IDialog::MessageType type);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<DialogManagerInterface> DialogManagerInterfacePtr;

} // namespace script

#endif /* _SCRIPT_DIALOG_INTERFACE_H_ */
