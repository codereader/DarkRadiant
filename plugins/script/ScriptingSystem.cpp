#include "ScriptingSystem.h"

#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"

#include "StartupListener.h"

#include "interfaces/MathInterface.h"
#include "interfaces/RegistryInterface.h"
#include "interfaces/RadiantInterface.h"
#include "interfaces/SceneGraphInterface.h"
#include "interfaces/EClassInterface.h"
#include "interfaces/SelectionInterface.h"
#include "interfaces/BrushInterface.h"
#include "interfaces/PatchInterface.h"
#include "interfaces/EntityInterface.h"
#include "interfaces/MapInterface.h"
#include "interfaces/CommandSystemInterface.h"
#include "interfaces/GameInterface.h"
#include "interfaces/FileSystemInterface.h"
#include "interfaces/GridInterface.h"
#include "interfaces/ShaderSystemInterface.h"

#include "ScriptWindow.h"

#include "os/path.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace fs = boost::filesystem;

namespace script {

ScriptingSystem::ScriptingSystem() :
	_outputWriter(false, _outputBuffer),
	_errorWriter(true, _errorBuffer),
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

ExecutionResultPtr ScriptingSystem::executeString(const std::string& scriptString)
{
	ExecutionResultPtr result(new ExecutionResult);

	result->errorOccurred = false;

	// Clear the output buffers before starting to execute
	_outputBuffer.clear();
	_errorBuffer.clear();

	try
	{
		// Attempt to run the specified script
		boost::python::object ignored = boost::python::exec(
			scriptString.c_str(),
			_mainNamespace,
			_globals
		);
	}
	catch (const boost::python::error_already_set&)
	{
		result->errorOccurred = true;

		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();
	}

	result->output += _outputBuffer + "\n";
	result->output += _errorBuffer + "\n";

	_outputBuffer.clear();
	_errorBuffer.clear();

	return result;
}

void ScriptingSystem::initialise() {
	// Add the registered interfaces
	try {
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

	// Add the scripting widget to the groupdialog
	GlobalGroupDialog().addPage(
		"ScriptWindow", "Script", "icon_script.png", 
		ScriptWindow::Instance().getWidget(), 
		"Script", "console"
	);
}

void ScriptingSystem::runScriptFile(const cmd::ArgumentList& args) {
	// Start the test script
	if (args.empty()) return;

	executeScriptFile(args[0].getString());
}

void ScriptingSystem::runScriptCommand(const cmd::ArgumentList& args) {
	// Start the test script
	if (args.empty()) return;

	executeCommand(args[0].getString());
}

void ScriptingSystem::reloadScriptsCmd(const cmd::ArgumentList& args) {
	reloadScripts();
}

void ScriptingSystem::executeCommand(const std::string& name) {
	// Sanity check
	if (!_initialised) {
		globalErrorStream() << "Cannot execute script command " << name 
			<< ", ScriptingSystem not initialised yet." << std::endl;
		return;
	}

	// Lookup the name
	ScriptCommandMap::iterator found = _commands.find(name);

	if (found == _commands.end()) {
		globalErrorStream() << "Couldn't find command " << name << std::endl;
		return;
	}

	// Set the execution flag in the global namespace
	_globals["__executeCommand__"] = true;

	// Execute the script file behind this command
	executeScriptFile(found->second->getFilename());
}

void ScriptingSystem::loadCommandScript(const std::string& scriptFilename) {
	try
	{
		// Create a new dictionary for the initialisation routine
		boost::python::dict locals;

		// Disable the flag for initialisation, just for sure
		locals["__executeCommand__"] = false;
		
		// Attempt to run the specified script
		boost::python::object ignored = boost::python::exec_file(
			(_scriptPath + scriptFilename).c_str(),
			_mainNamespace,
			locals	// pass the new dictionary for the locals
		);
		
		std::string cmdName = boost::python::extract<std::string>(locals["__commandName__"]);
		
		if (!cmdName.empty()) {
			// Successfully retrieved the command
			ScriptCommandPtr cmd(new ScriptCommand(cmdName, scriptFilename));

			// Try to register this named command
			std::pair<ScriptCommandMap::iterator, bool> result = _commands.insert(
				ScriptCommandMap::value_type(cmdName, cmd)
			);

			// Result.second is TRUE if the insert succeeded
			if (result.second) {
				globalOutputStream() << "Registered script file " << scriptFilename 
					<< " as " << cmdName << std::endl;
			}
			else {
				globalErrorStream() << "Error in " << scriptFilename << ": Script command " 
					<< cmdName << " has already been registered in " 
					<< _commands[cmdName]->getFilename() << std::endl;
			}
		}
	}
	catch (const boost::python::error_already_set&) {
		globalErrorStream() << "Script file " << scriptFilename 
			<< " is not a valid command." << std::endl;

		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();

		// Python is usually not appending line feeds...
		globalOutputStream() << std::endl;
	}
}

void ScriptingSystem::reloadScripts() {
	// Release all previously allocated commands
	_commands.clear();

	// Initialise the search's starting point
	fs::path start = fs::path(_scriptPath) / "commands/";

	if (!fs::exists(start)) {
		globalWarningStream() << "Couldn't find scripts folder: " << start.string() << std::endl;
		return;
	}

	for (fs::recursive_directory_iterator it(start); 
		 it != fs::recursive_directory_iterator(); ++it)
	{
		// Get the candidate
		const fs::path& candidate = *it;

		if (fs::is_directory(candidate)) continue;

		std::string extension = os::getExtension(candidate.string());
		boost::algorithm::to_lower(extension);

		if (extension != "py") continue;

		// Script file found, construct a new command
		loadCommandScript(os::getRelativePath(candidate.string(), _scriptPath));
	}

	globalOutputStream() << "ScriptModule: Found " << _commands.size() << " commands." << std::endl;
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
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
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
#if defined(POSIX) && defined(PKGLIBDIR)
   _scriptPath = std::string(PKGLIBDIR) + "/scripts/";
#else
	_scriptPath = ctx.getApplicationPath() + "scripts/";
#endif

	// start the python interpreter
	Py_Initialize();

	globalOutputStream() << getName() << ": Python interpreter initialised.\n";

	// Initialise the boost::python objects
	_mainModule = boost::python::import("__main__");
	_mainNamespace = _mainModule.attr("__dict__");
	
	try {
		// Construct the console writer interface
		PythonConsoleWriterClass consoleWriter("PythonConsoleWriter", boost::python::init<bool, std::string&>());
		consoleWriter.def("write", &PythonConsoleWriter::write);

		// Declare the interface to python
		_mainNamespace["PythonConsoleWriter"] = consoleWriter;
		
		// Redirect stdio output to our local ConsoleWriter instances
		boost::python::import("sys").attr("stderr") = boost::python::ptr(&_errorWriter);
		boost::python::import("sys").attr("stdout") = boost::python::ptr(&_outputWriter);
	}
	catch (const boost::python::error_already_set&) {
		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();

		// Python is usually not appending line feeds...
		globalOutputStream() << std::endl;
	}

	// Add the built-in interfaces (the order is important, as we don't have dependency-resolution yet)
	addInterface("Math", MathInterfacePtr(new MathInterface));
	addInterface("GameManager", GameInterfacePtr(new GameInterface));
	addInterface("CommandSystem", CommandSystemInterfacePtr(new CommandSystemInterface));
	addInterface("SceneGraph", SceneGraphInterfacePtr(new SceneGraphInterface));
	addInterface("GlobalRegistry", RegistryInterfacePtr(new RegistryInterface));
	addInterface("GlobalEntityClassManager", EClassManagerInterfacePtr(new EClassManagerInterface));
	addInterface("GlobalSelectionSystem", SelectionInterfacePtr(new SelectionInterface));
	addInterface("Brush", BrushInterfacePtr(new BrushInterface));
	addInterface("Patch", PatchInterfacePtr(new PatchInterface));
	addInterface("Entity", EntityInterfacePtr(new EntityInterface));
	addInterface("Radiant", RadiantInterfacePtr(new RadiantInterface));
	addInterface("Map", MapInterfacePtr(new MapInterface));
	addInterface("FileSystem", FileSystemInterfacePtr(new FileSystemInterface));
	addInterface("Grid", GridInterfacePtr(new GridInterface));
	addInterface("ShaderSystem", ShaderSystemInterfacePtr(new ShaderSystemInterface));

	GlobalCommandSystem().addCommand(
		"RunScript", 
		boost::bind(&ScriptingSystem::runScriptFile, this, _1),
		cmd::ARGTYPE_STRING
	);

	GlobalCommandSystem().addCommand(
		"ReloadScripts", 
		boost::bind(&ScriptingSystem::reloadScriptsCmd, this, _1)
	);

	GlobalCommandSystem().addCommand(
		"RunScriptCommand", 
		boost::bind(&ScriptingSystem::runScriptCommand, this, _1),
		cmd::ARGTYPE_STRING
	);

	// Search script folder for commands
	reloadScripts();

	// Register each script command in the CommandSystem
	for (ScriptCommandMap::const_iterator i = _commands.begin(); i != _commands.end(); ++i)
	{
		GlobalCommandSystem().addStatement(i->first, "RunScriptCommand " + i->first);

		// Add an event as well (for keyboard shortcuts)
		GlobalEventManager().addCommand(i->first, i->first);
	}

	// Bind the reloadscripts command to the menu
	GlobalEventManager().addCommand("ReloadScripts", "ReloadScripts");

	// Add the menu item
	IMenuManager& mm = GlobalUIManager().getMenuManager();
	mm.insert("main/file/refreshShaders", 	// menu location path
			"ReloadScripts", // name
			ui::menuItem,	// type
			"Reload Scripts",	// caption
			"",	// icon
			"ReloadScripts"); // event name
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
