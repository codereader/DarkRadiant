#ifndef _GUI_VARIABLE_H_
#define _GUI_VARIABLE_H_

#include <string>
#include <boost/shared_ptr.hpp>

namespace gui
{

class Gui;
class GuiWindowDef;

// An abstract variable wrapping around a target object in a GUI.
// This can be a GUI state variable or a windowDef property
class Variable
{
public:
	virtual ~Variable() {}

	// Assign a value to this Variable (returns TRUE on success)
	virtual bool assignValueFromString(const std::string& val) = 0;
};
typedef boost::shared_ptr<Variable> VariablePtr;

// A variable pointing to a specific item of a windowDef
class WindowDefVariable :
	public Variable
{
private:
	// The parent windowDef of this variable
	GuiWindowDef& _windowDef;

	// Variable name (in the windowDef
	std::string _name;

public:
	WindowDefVariable(GuiWindowDef& windowDef, const std::string& name);

	// Assign a value to this Variable (returns TRUE on success)
	bool assignValueFromString(const std::string& val);
};

// A variable pointing to a GUI state variable
class GuiStateVariable :
	public Variable
{
private:
	// The GUi the state variable is located in
	Gui& _gui;

	// State key (name)
	std::string _key;

public:
	GuiStateVariable(Gui& gui, const std::string& key);

	bool assignValueFromString(const std::string& val);
};

} // namespace

#endif /* _GUI_VARIABLE_H_ */
