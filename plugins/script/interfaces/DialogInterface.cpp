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
	nspace["Dialog"] = boost::python::class_<ScriptDialog>("Dialog", 
		boost::python::init<const ui::IDialogPtr&>())
		.def("setTitle", &ScriptDialog::setTitle)
		.def("setDialogType", &ScriptDialog::setDialogType)
		.def("run", &ScriptDialog::run)
		.def("destroy", &ScriptDialog::destroy)
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalDialogManager"] = boost::python::class_<DialogManagerInterface>("GlobalDialogManager")
		.def("createDialog", &DialogManagerInterface::createDialog)
	;

	// Now point the Python variable "GlobalDialogManager" to this instance
	nspace["GlobalDialogManager"] = boost::python::ptr(this);
}


} // namespace script
