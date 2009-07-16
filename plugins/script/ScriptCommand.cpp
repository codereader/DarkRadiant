#include "ScriptCommand.h"

#include "icommandsystem.h"
#include "ieventmanager.h"

namespace script {

ScriptCommand::ScriptCommand(const std::string& name, const std::string& scriptFilename) :
	_name(name),
	_scriptFilename(scriptFilename)
{
	// Register this with the command system
	GlobalCommandSystem().addStatement(_name, "RunScriptCommand " + _name);

	// Add an event as well (for keyboard shortcuts)
	GlobalEventManager().addCommand(_name, _name);
}

ScriptCommand::~ScriptCommand()
{
	// Add an event as well (for keyboard shortcuts)
	GlobalEventManager().removeEvent(_name);

	GlobalCommandSystem().removeCommand(_name);
}

} // namespace script
