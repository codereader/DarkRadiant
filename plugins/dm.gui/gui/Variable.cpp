#include "Variable.h"

#include "Gui.h"
#include "GuiWindowDef.h"
#include <boost/algorithm/string/case_conv.hpp>

namespace gui
{

WindowDefVariable::WindowDefVariable(GuiWindowDef& windowDef,
									 const std::string& name) :
	_windowDef(windowDef),
	_name(boost::algorithm::to_lower_copy(name))
{}

// Assign a value to this Variable (returns TRUE on success)
bool WindowDefVariable::assignValueFromString(const std::string& val)
{
	if (_name.empty()) return false;

	if (_name == "text")
	{
		_windowDef.setText(val);
		return true;
	}
	else
	{
		// TODO
		return false;
	}
}

GuiStateVariable::GuiStateVariable(Gui& gui, const std::string& key) :
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
