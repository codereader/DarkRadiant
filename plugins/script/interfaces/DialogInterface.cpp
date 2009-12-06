#include "DialogInterface.h"

#include "iuimanager.h"

namespace script
{

ScriptDialog DialogManagerInterface::createDialog(const std::string& title, ui::IDialog::Type type)
{
	return ScriptDialog(GlobalUIManager().getDialogManager().createDialog(title, type));
}

// IScriptInterface implementation
void DialogManagerInterface::registerInterface(boost::python::object& nspace)
{
	// Add the declaration for the IDialog class
	boost::python::class_<ScriptDialog> dialog("Dialog", 
		boost::python::init<const ui::IDialogPtr&>());

	// Add the methods to the dialog object
	dialog
		.def("setTitle", &ScriptDialog::setTitle)
		.def("setDialogType", &ScriptDialog::setDialogType)
		.def("run", &ScriptDialog::run)
		.def("destroy", &ScriptDialog::destroy)
	;

	// Register the dialog class name
	nspace["Dialog"] = dialog;

	// Switch to the Dialog's class scope
	boost::python::scope dialogScope( dialog );

	// Expose the enums in the Dialog's scope
	boost::python::enum_<ui::IDialog::Result>("Result")
		.value("CANCELLED", ui::IDialog::RESULT_CANCELLED)
		.value("OK", ui::IDialog::RESULT_OK)
		.value("NO", ui::IDialog::RESULT_NO)
		.value("YES", ui::IDialog::RESULT_YES)
		.export_values()
	;

	boost::python::enum_<ui::IDialog::Type>("Type")
		.value("OK", ui::IDialog::DIALOG_OK)
		.value("OK_CANCEL", ui::IDialog::DIALOG_OK_CANCEL)
		.value("YES_NO", ui::IDialog::DIALOG_YES_NO)
		.export_values()
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalDialogManager"] = boost::python::class_<DialogManagerInterface>("GlobalDialogManager")
		.def("createDialog", &DialogManagerInterface::createDialog)
	;

	// Now point the Python variable "GlobalDialogManager" to this instance
	nspace["GlobalDialogManager"] = boost::python::ptr(this);
}


} // namespace script
