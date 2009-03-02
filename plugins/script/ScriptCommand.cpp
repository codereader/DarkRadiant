#include "ScriptCommand.h"

#include "icommandsystem.h"

namespace script {

ScriptCommand::ScriptCommand(const std::string& name, const std::string& scriptFilename) :
	_name(name),
	_scriptFilename(scriptFilename)
{
	// Register this with the command system
	GlobalCommandSystem().addStatement(_name, "RunScriptCommand " + _name);
}

ScriptCommand::~ScriptCommand() {
	GlobalCommandSystem().removeCommand(_name);
}

} // namespace script
