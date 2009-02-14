#ifndef _SCRIPTING_SYSTEM_H_
#define _SCRIPTING_SYSTEM_H_

#include "imodule.h"
#include "PythonConsoleWriter.h"

namespace script {

class ScriptingSystem :
	public RegisterableModule
{
	PythonConsoleWriter _outputWriter;
	PythonConsoleWriter _errorWriter;
public:
	ScriptingSystem();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<ScriptingSystem> ScriptingSystemPtr;

} // namespace script

#endif /* _SCRIPTING_SYSTEM_H_ */
