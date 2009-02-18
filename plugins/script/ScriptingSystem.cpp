#include "ScriptingSystem.h"

#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"

#include "StartupListener.h"

#include "interfaces/MathInterface.h"
#include "interfaces/RegistryInterface.h"
#include "interfaces/RadiantInterface.h"
#include "interfaces/SceneGraphInterface.h"
#include "interfaces/EClassInterface.h"
#include "interfaces/SelectionInterface.h"
#include "interfaces/BrushInterface.h"
#include "interfaces/PatchInterface.h"

namespace script {

ScriptingSystem::ScriptingSystem() :
	_outputWriter(false),
	_errorWriter(true),
	_initialised(false)
{}

void ScriptingSystem::addInterface(const std::string& name, const IScriptInterfacePtr& iface) {
	// Check if exists
	if (interfaceExists(name)) {
		globalErrorStream() << "Cannot add script interface " << name 
			<< ", this interface is already registered." << std::endl;
		return;
	}

	// Try to insert
	_interfaces.push_back(
		std::make_pair<std::string, IScriptInterfacePtr>(name, iface)
	);
	
	if (_initialised) {
		// Add the interface at once, all the others are already added
		iface->registerInterface(_mainNamespace);
	}
}

bool ScriptingSystem::interfaceExists(const std::string& name) {
	// Traverse the interface list 
	for (Interfaces::iterator i = _interfaces.begin(); i != _interfaces.end(); ++i) {
		if (i->first == name) {
			return true;
		}
	}

	return false;
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

void ScriptingSystem::runTestScript() {
	// Start the test script
	executeScriptFile("test.py");
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
		_dependencies.insert(MODULE_EVENTMANAGER);
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
	addInterface("Math", MathInterfacePtr(new MathInterface));
	addInterface("Radiant", RadiantInterfacePtr(new RadiantInterface));
	addInterface("SceneGraph", SceneGraphInterfacePtr(new SceneGraphInterface));
	addInterface("GlobalRegistry", RegistryInterfacePtr(new RegistryInterface));
	addInterface("GlobalEntityClassManager", EClassManagerInterfacePtr(new EClassManagerInterface));
	addInterface("GlobalSelectionSystem", SelectionInterfacePtr(new SelectionInterface));
	addInterface("Brush", BrushInterfacePtr(new BrushInterface));
	addInterface("Patch", PatchInterfacePtr(new PatchInterface));

	GlobalEventManager().addCommand("RunTestScript", MemberCaller<ScriptingSystem, &ScriptingSystem::runTestScript>(*this));
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
