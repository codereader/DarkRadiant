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

		try
		{
			WindowVariable<Vector4>& vec4var = dynamic_cast<WindowVariable<Vector4>&>(variable);
		}
		catch (std::bad_cast&)
		{
			// Must be an ordinary value
		}
		// TODO: Switch on value type

		return true;
	}
	catch (std::invalid_argument&)
	{
		rError() << "[GUI Script]: Cannot assign value to unknown variable: " << 
			_windowDef.name << "::" << _name << std::endl;
		return false;
	}

	if (_name == "text")
	{
		_windowDef.text.setValue(ConstantExpression<std::string>::Create(val));
		return true;
	}
    else if (_name == "background")
	{
        if (_windowDef.background.getValue() != val)
        {
            // Reset the material reference when changing background
            _windowDef.background.setValue(ConstantExpression<std::string>::Create(val));
            _windowDef.backgroundShader.reset();
        }
		return true;
	}
	else
	{
		// TODO
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
