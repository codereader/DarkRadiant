#ifndef _SCRIPT_COMMAND_H_
#define _SCRIPT_COMMAND_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

namespace script {

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

	const std::string& getFilename() {
		return _scriptFilename;
	}

	const std::string& getDisplayName() {
		return _displayName;
	}
};
typedef boost::shared_ptr<ScriptCommand> ScriptCommandPtr;

// A mapping of named script commands
typedef std::map<std::string, ScriptCommandPtr> ScriptCommandMap;

} // namespace script

#endif /* _SCRIPT_COMMAND_H_ */
