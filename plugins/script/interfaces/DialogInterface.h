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

	void setDialogType(ui::IDialog::Type type)
	{
		if (_dialog != NULL) _dialog->setDialogType(type);
	}

	ui::IDialog::Result run()
	{
		return (_dialog != NULL) ? _dialog->run() : ui::IDialog::RESULT_CANCELLED;
	}

	void destroy()
	{
		if (_dialog != NULL) _dialog->destroy();
	}
};

/**
 * greebo: This class provides the script interface for the DialogManager class (UIManager module).
 */
class DialogManagerInterface :
	public IScriptInterface
{
public:
	ScriptDialog createDialog(const std::string& title, ui::IDialog::Type type);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<DialogManagerInterface> DialogManagerInterfacePtr;

} // namespace script

#endif /* _SCRIPT_DIALOG_INTERFACE_H_ */
