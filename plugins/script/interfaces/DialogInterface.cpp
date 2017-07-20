#include "DialogInterface.h"

#include <pybind11/pybind11.h>
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
void DialogManagerInterface::registerInterface(py::module& scope, py::dict& globals)
{
	py::class_<DialogManagerInterface> dialogMgr(scope, "DialogManager");
	dialogMgr.def("createDialog", &DialogManagerInterface::createDialog);
	dialogMgr.def("createMessageBox", &DialogManagerInterface::createMessageBox);

	// Now point the Python variable "GlobalDialogManager" to this instance
	globals["GlobalDialogManager"] = this;

	// Add the declaration for the IDialog class
	py::class_<ScriptDialog> dialog(scope, "Dialog");
	dialog.def(py::init<const ui::IDialogPtr&>());

	// Add the methods to the dialog object
	dialog.def("setTitle", &ScriptDialog::setTitle);
	dialog.def("run", &ScriptDialog::run);
	dialog.def("addLabel", &ScriptDialog::addLabel);
	dialog.def("addComboBox", &ScriptDialog::addComboBox);
	dialog.def("addEntryBox", &ScriptDialog::addEntryBox);
	dialog.def("addPathEntry", &ScriptDialog::addPathEntry);
	dialog.def("addSpinButton", &ScriptDialog::addSpinButton);
	dialog.def("addCheckbox", &ScriptDialog::addCheckbox);
	dialog.def("getElementValue", &ScriptDialog::getElementValue);
	dialog.def("setElementValue", &ScriptDialog::setElementValue);

	// Expose the enums in the Dialog's scope
	py::enum_<ui::IDialog::Result>(scope, "Result")
		.value("CANCELLED", ui::IDialog::RESULT_CANCELLED)
		.value("OK", ui::IDialog::RESULT_OK)
		.value("NO", ui::IDialog::RESULT_NO)
		.value("YES", ui::IDialog::RESULT_YES)
		.export_values();

	py::enum_<ui::IDialog::MessageType>(scope, "MessageType")
		.value("CONFIRM", ui::IDialog::MESSAGE_CONFIRM)
		.value("ASK", ui::IDialog::MESSAGE_ASK)
		.value("WARNING", ui::IDialog::MESSAGE_WARNING)
		.value("ERROR", ui::IDialog::MESSAGE_ERROR)
		.value("YESNOCANCEL", ui::IDialog::MESSAGE_YESNOCANCEL)
		.export_values();
}


} // namespace script
