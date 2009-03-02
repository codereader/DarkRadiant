#ifndef _SCRIPT_COMMAND_H_
#define _SCRIPT_COMMAND_H_

#include <string>
#include <boost/shared_ptr.hpp>

namespace script {

// Forward decl.
class ScriptingSystem;

class ScriptCommand
{
	// The name of this command
	std::string _name;

	// The script file name to execute (relative to scripts/ folder)
	std::string _scriptFilename;

public:
	ScriptCommand(const std::string& name, const std::string& scriptFilename);

	~ScriptCommand();

	const std::string& getFilename() {
		return _scriptFilename;
	}
};
typedef boost::shared_ptr<ScriptCommand> ScriptCommandPtr;

} // namespace script

#endif /* _SCRIPT_COMMAND_H_ */
