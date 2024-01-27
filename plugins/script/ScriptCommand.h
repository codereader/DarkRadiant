#pragma once

#include "iscript.h"
#include <string>
#include <memory>

namespace script 
{

// Forward decl.
class ScriptingSystem;

class ScriptCommand :
	public IScriptCommand
{
private:
	// The name of this command
	std::string _name;

	// Caption for the menus
	std::string _displayName;

	// The script file name to execute (relative to scripts/ folder)
	std::string _scriptFilename;

public:
    using Ptr = std::shared_ptr<ScriptCommand>;

	ScriptCommand(const std::string& name,
				  const std::string& displayName,
				  const std::string& scriptFilename);

	~ScriptCommand() override;

    const std::string& getName() const override
    {
        return _name;
    }

	const std::string& getFilename() const override
    {
		return _scriptFilename;
	}

	const std::string& getDisplayName() const override
    {
		return _displayName;
	}
};

} // namespace script
