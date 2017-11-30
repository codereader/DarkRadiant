#include "Variable.h"

#include "igui.h"
#include "GuiExpression.h"
#include "string/case_conv.h"

namespace gui
{

AssignableWindowVariable::AssignableWindowVariable(IGuiWindowDef& windowDef,
									 const std::string& name) :
	_windowDef(windowDef),
	_name(string::to_lower_copy(name))
{}

// Assign a value to this Variable (returns TRUE on success)
bool AssignableWindowVariable::assignValueFromString(const std::string& val)
{
	if (_name.empty()) return false;

	// Lookup the target window variable by name
	try
	{
		IWindowVariable& variable = _windowDef.findVariableByName(_name);
		variable.setValueFromString(val);

		return true;
	}
	catch (std::invalid_argument&)
	{
		rError() << "[GUI Script]: Cannot assign value to unknown variable: " << 
			_windowDef.name << "::" << _name << std::endl;
		return false;
	}
}

GuiStateVariable::GuiStateVariable(IGui& gui, const std::string& key) :
	_gui(gui),
	_key(key)
{}

// Assign a value to this Variable (returns TRUE on success)
bool GuiStateVariable::assignValueFromString(const std::string& val)
{
	_gui.setStateString(_key, val);

	return true; // always succeeds
}

} // namespace
