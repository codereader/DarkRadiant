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
