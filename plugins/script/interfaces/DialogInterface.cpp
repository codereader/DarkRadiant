#include "DialogInterface.h"

#include "iuimanager.h"

namespace script
{

ScriptDialog DialogManagerInterface::createDialog(const std::string& title)
{
	return ScriptDialog(GlobalUIManager().getDialogManager().createDialog(title));
}

ScriptDialog DialogManagerInterface::createMessageBox(const std::string& title, 
													  const std::string& text, 
													  ui::IDialog::MessageType type)
{
	return ScriptDialog(GlobalUIManager().getDialogManager().createMessageBox(title, text, type));
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
		.def("run", &ScriptDialog::run)
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

	boost::python::enum_<ui::IDialog::MessageType>("MessageType")
		.value("CONFIRM", ui::IDialog::MESSAGE_CONFIRM)
		.value("ASK", ui::IDialog::MESSAGE_ASK)
		.value("WARNING", ui::IDialog::MESSAGE_WARNING)
		.value("ERROR", ui::IDialog::MESSAGE_ERROR)
		.export_values()
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalDialogManager"] = boost::python::class_<DialogManagerInterface>("GlobalDialogManager")
		.def("createDialog", &DialogManagerInterface::createDialog)
		.def("createMessageBox", &DialogManagerInterface::createMessageBox)
	;

	// Now point the Python variable "GlobalDialogManager" to this instance
	nspace["GlobalDialogManager"] = boost::python::ptr(this);
}


} // namespace script
