#pragma once

#include <string>
#include <memory>

namespace gui
{

class IGui;
class IGuiWindowDef;

// An abstract variable wrapping around a target object in a GUI.
// This can be a GUI state variable or a windowDef property
class Variable
{
public:
	virtual ~Variable() {}

	// Assign a value to this Variable (returns TRUE on success)
	virtual bool assignValueFromString(const std::string& val) = 0;
};
typedef std::shared_ptr<Variable> VariablePtr;

// A variable pointing to a specific item of a windowDef
class WindowDefVariable :
	public Variable
{
private:
	// The parent windowDef of this variable
	IGuiWindowDef& _windowDef;

	// Variable name (in the windowDef
	std::string _name;

public:
	WindowDefVariable(IGuiWindowDef& windowDef, const std::string& name);

	// Assign a value to this Variable (returns TRUE on success)
	bool assignValueFromString(const std::string& val);
};

// A variable pointing to a GUI state variable
class GuiStateVariable :
	public Variable
{
private:
	// The GUi the state variable is located in
	IGui& _gui;

	// State key (name)
	std::string _key;

public:
	GuiStateVariable(IGui& gui, const std::string& key);

	bool assignValueFromString(const std::string& val);
};

} // namespace
