#ifndef _SCRIPTING_SYSTEM_H_
#define _SCRIPTING_SYSTEM_H_

#include <boost/python.hpp>
#include <map>

#include "iscript.h"
#include "PythonConsoleWriter.h"

namespace script {

// Forward declaration
class StartupListener;
typedef boost::shared_ptr<StartupListener> StartupListenerPtr;

class ScriptingSystem :
	public IScriptingSystem
{
	PythonConsoleWriter _outputWriter;
	PythonConsoleWriter _errorWriter;

	bool _initialised;

	typedef std::map<std::string, IScriptInterfacePtr> Interfaces;
	Interfaces _interfaces;

	boost::python::object _mainModule;
	boost::python::object _mainNamespace;
	boost::python::dict _globals;

	StartupListenerPtr _startupListener;

	// The path where the script files are hosted
	std::string _scriptPath;

public:
	ScriptingSystem();

	// Adds a script interface to this system
	void addInterface(const std::string& name, const IScriptInterfacePtr& iface);

	// Executes a script file
	void executeScriptFile(const std::string& filename);

	/**
	 * This actually initialises the Scripting System, adding all
	 * registered interfaces to the Python context. After this call
	 * the scripting system is ready for use.
	 *
	 * This method also invokes "scripts/init.py" when done.
	 */
	void initialise();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void ScriptingSystem::shutdownModule();
};
typedef boost::shared_ptr<ScriptingSystem> ScriptingSystemPtr;

} // namespace script

#endif /* _SCRIPTING_SYSTEM_H_ */
