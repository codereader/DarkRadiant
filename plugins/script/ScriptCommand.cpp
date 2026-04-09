#include "ScriptCommand.h"

#include "icommandsystem.h"

namespace script
{

ScriptCommand::ScriptCommand(const std::string& name,
							 const std::string& displayName,
							 const std::string& scriptFilename,
							 const std::string& basePath) :
	_name(name),
	_displayName(displayName),
	_scriptFilename(scriptFilename),
	_basePath(basePath)
{
	// Register this with the command system
	GlobalCommandSystem().addStatement(_name, "RunScriptCommand '" + _name + "'", false);
}

ScriptCommand::~ScriptCommand()
{
	GlobalCommandSystem().removeCommand(_name);
}

} // namespace script
