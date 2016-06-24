#pragma once

#include <string>
#include <map>
#include <memory>

namespace script 
{

// Forward decl.
class ScriptingSystem;

class ScriptCommand
{
	// The name of this command
	std::string _name;

	// Caption for the menus
	std::string _displayName;

	// The script file name to execute (relative to scripts/ folder)
	std::string _scriptFilename;

public:
	ScriptCommand(const std::string& name,
				  const std::string& displayName,
				  const std::string& scriptFilename);

	~ScriptCommand();

    const std::string& getName()
    {
        return _name;
    }

	const std::string& getFilename()
    {
		return _scriptFilename;
	}

	const std::string& getDisplayName()
    {
		return _displayName;
	}
};
typedef std::shared_ptr<ScriptCommand> ScriptCommandPtr;

// A mapping of named script commands
typedef std::map<std::string, ScriptCommandPtr> ScriptCommandMap;

} // namespace script
