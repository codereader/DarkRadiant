#include "ScriptingSystem.h"

#include "itextstream.h"
#include "iradiant.h"

#include "StartupListener.h"

#include "interfaces/RegistryInterface.h"
#include "interfaces/RadiantInterface.h"
#include "interfaces/EClassInterface.h"

namespace script {

ScriptingSystem::ScriptingSystem() :
	_outputWriter(false),
	_errorWriter(true),
	_initialised(false)
{}

void ScriptingSystem::addInterface(const std::string& name, const IScriptInterfacePtr& iface) {
	// Try to insert
	std::pair<Interfaces::iterator, bool> result = _interfaces.insert(
		Interfaces::value_type(name, iface)
	);

	if (!result.second) {
		globalErrorStream() << "Cannot add script interface " << name 
			<< ", this interface is already registered." << std::endl;
		return;
	}

	if (_initialised) {
		// Add the interface at once, all the others are already added
		iface->registerInterface(_mainNamespace);
	}
}

void ScriptingSystem::executeScriptFile(const std::string& filename) {
	try
	{
		// Attempt to run the specified script
		boost::python::object ignored = boost::python::exec_file(
			(_scriptPath + filename).c_str(),
			_mainNamespace,
			_globals
		);
	}
	catch (const boost::python::error_already_set&) {
		globalErrorStream() << "Error while executing file: " 
					<< filename << ": " << std::endl;

		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();

		// Python is usually not appending line feeds...
		globalOutputStream() << std::endl;
	}
}

void ScriptingSystem::initialise() {
	// start the python interpreter
	Py_Initialize();

	globalOutputStream() << getName() << ": Python interpreter initialised.\n";

	// Initialise the boost::python objects
	_mainModule = boost::python::import("__main__");
	_mainNamespace = _mainModule.attr("__dict__");
	
	try {
		// Construct the console writer interface
		PythonConsoleWriterClass consoleWriter("PythonConsoleWriter", boost::python::init<bool>());
		consoleWriter.def("write", &PythonConsoleWriter::write);

		// Declare the interface to python
		_mainNamespace["PythonConsoleWriter"] = consoleWriter;
		
		// Redirect stdio output to our local ConsoleWriter instances
		boost::python::import("sys").attr("stderr") = _errorWriter;
		boost::python::import("sys").attr("stdout") = _outputWriter; 

		// Add the registered interface
		for (Interfaces::iterator i = _interfaces.begin(); i != _interfaces.end(); ++i) {
			// Handle each interface in its own try/catch block
			try 
			{
				i->second->registerInterface(_mainNamespace);
			}
			catch (const boost::python::error_already_set&)
			{
				globalErrorStream() << "Error while initialising interface " 
					<< i->first << ": " << std::endl;

				PyErr_Print();
				PyErr_Clear();

				globalOutputStream() << std::endl;
			}
		}
	}
	catch (const boost::python::error_already_set&) {
		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();

		// Python is usually not appending line feeds...
		globalOutputStream() << std::endl;
	}

	_initialised = true;

	// Start the init script
	executeScriptFile("init.py");
}

// RegisterableModule implementation
const std::string& ScriptingSystem::getName() const {
	static std::string _name(MODULE_SCRIPTING_SYSTEM);
	return _name;
}

const StringSet& ScriptingSystem::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_RADIANT);
	}

	return _dependencies;
}

void ScriptingSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << getName() << "::initialiseModule called.\n";

	// Subscribe to get notified as soon as Radiant is fully initialised
	_startupListener = StartupListenerPtr(new StartupListener(*this));
	GlobalRadiant().addEventListener(_startupListener);

	// Construct the script path
	_scriptPath = ctx.getApplicationPath() + "scripts/";

	// Add the built-in interfaces
	addInterface("Radiant", RadiantInterfacePtr(new RadiantInterface));
	addInterface("GlobalRegistry", RegistryInterfacePtr(new RegistryInterface));
	addInterface("GlobalEntityClassManager", EClassManagerInterfacePtr(new EClassManagerInterface));
}

void ScriptingSystem::shutdownModule() {
	globalOutputStream() << getName() << "::shutdownModule called.\n";

	_scriptPath.clear();
	_startupListener = StartupListenerPtr();

	// Free all interfaces
	_interfaces.clear();

	_initialised = false;

	Py_Finalize();
}

} // namespace script
