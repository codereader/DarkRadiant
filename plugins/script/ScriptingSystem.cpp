#include "ScriptingSystem.h"

#include "i18n.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"

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
#include "interfaces/ModelInterface.h"
#include "interfaces/SkinInterface.h"
#include "interfaces/SoundInterface.h"
#include "interfaces/DialogInterface.h"
#include "interfaces/SelectionSetInterface.h"

#include "ScriptWindow.h"
#include "SceneNodeBuffer.h"

#include "os/path.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

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
		rError() << "Cannot add script interface " << name
			<< ", this interface is already registered." << std::endl;
		return;
	}

	// Try to insert
	_interfaces.push_back(NamedInterface(name, iface));

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
		rError() << "Error while executing file: "
					<< filename << ": " << std::endl;

		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();

		// Python is usually not appending line feeds...
		rMessage() << std::endl;
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

void ScriptingSystem::initialise()
{
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
				rError() << "Error while initialising interface "
					<< i->first << ": " << std::endl;

				PyErr_Print();
				PyErr_Clear();

				rMessage() << std::endl;
			}
		}
	}
	catch (const boost::python::error_already_set&) {
		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();

		// Python is usually not appending line feeds...
		rMessage() << std::endl;
	}

	_initialised = true;

	// Start the init script
	executeScriptFile("init.py");

	// Ensure the singleton instance exists
	ScriptWindow::create();

	// Add the scripting widget to the groupdialog
	GlobalGroupDialog().addPage(
		"ScriptWindow", _("Script"), "icon_script.png",
		*ScriptWindow::InstancePtr().get(),
		_("Script"), "console"
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
		rError() << "Cannot execute script command " << name
			<< ", ScriptingSystem not initialised yet." << std::endl;
		return;
	}

	// Lookup the name
	ScriptCommandMap::iterator found = _commands.find(name);

	if (found == _commands.end()) {
		rError() << "Couldn't find command " << name << std::endl;
		return;
	}

	// Set the execution flag in the global namespace
	_globals["__executeCommand__"] = true;

	// Execute the script file behind this command
	executeScriptFile(found->second->getFilename());
}

void ScriptingSystem::loadCommandScript(const std::string& scriptFilename)
{
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

		std::string cmdName;
		std::string cmdDisplayName;

		if (locals.has_key("__commandName__"))
		{
			cmdName = boost::python::extract<std::string>(locals["__commandName__"]);
		}

		if (locals.has_key("__commandDisplayName__"))
		{
			cmdDisplayName = boost::python::extract<std::string>(locals["__commandDisplayName__"]);
		}

		if (!cmdName.empty())
		{
			if (cmdDisplayName.empty())
			{
				cmdDisplayName = cmdName;
			}

			// Successfully retrieved the command
			ScriptCommandPtr cmd(new ScriptCommand(cmdName, cmdDisplayName, scriptFilename));

			// Try to register this named command
			std::pair<ScriptCommandMap::iterator, bool> result = _commands.insert(
				ScriptCommandMap::value_type(cmdName, cmd)
			);

			// Result.second is TRUE if the insert succeeded
			if (result.second) {
				rMessage() << "Registered script file " << scriptFilename
					<< " as " << cmdName << std::endl;
			}
			else {
				rError() << "Error in " << scriptFilename << ": Script command "
					<< cmdName << " has already been registered in "
					<< _commands[cmdName]->getFilename() << std::endl;
			}
		}
	}
	catch (const boost::python::error_already_set&) {
		rError() << "Script file " << scriptFilename
			<< " is not a valid command." << std::endl;

		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();

		// Python is usually not appending line feeds...
		rMessage() << std::endl;
	}
}

void ScriptingSystem::reloadScripts()
{
	// Release all previously allocated commands
	_commands.clear();

	// Initialise the search's starting point
	fs::path start = fs::path(_scriptPath) / "commands/";

	if (!fs::exists(start)) {
		rWarning() << "Couldn't find scripts folder: " << start.string() << std::endl;
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

	rMessage() << "ScriptModule: Found " << _commands.size() << " commands." << std::endl;

	// Re-create the script menu
	_scriptMenu.reset();

	_scriptMenu = ui::ScriptMenuPtr(new ui::ScriptMenu(_commands));
}

// RegisterableModule implementation
const std::string& ScriptingSystem::getName() const
{
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

void ScriptingSystem::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Subscribe to get notified as soon as Radiant is fully initialised
	GlobalRadiant().signal_radiantStarted().connect(
        sigc::mem_fun(this, &ScriptingSystem::initialise)
    );

	// Construct the script path
#if defined(POSIX) && defined(PKGLIBDIR)
   _scriptPath = std::string(PKGLIBDIR) + "/scripts/";
#else
	_scriptPath = ctx.getApplicationPath() + "scripts/";
#endif

	// start the python interpreter
	Py_Initialize();

	rMessage() << getName() << ": Python interpreter initialised." << std::endl;

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
		rMessage() << std::endl;
	}

	// Declare the std::vector<std::string> object to Python, this is used several times
	_mainNamespace["StringVector"] = boost::python::class_< std::vector<std::string> >("StringVector")
		.def(boost::python::vector_indexing_suite<std::vector<std::string>, true>())
	;

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
	addInterface("Model", ModelInterfacePtr(new ModelInterface));
	addInterface("ModelSkinCacheInterface", ModelSkinCacheInterfacePtr(new ModelSkinCacheInterface));
	addInterface("SoundManager", SoundManagerInterfacePtr(new SoundManagerInterface));
	addInterface("DialogInterface", DialogManagerInterfacePtr(new DialogManagerInterface));
	addInterface("SelectionSetInterface", SelectionSetInterfacePtr(new SelectionSetInterface));

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

	// Bind the reloadscripts command to the menu
	GlobalEventManager().addCommand("ReloadScripts", "ReloadScripts");

	// Add the menu item
	IMenuManager& mm = GlobalUIManager().getMenuManager();
	mm.insert("main/file/refreshShaders", 	// menu location path
			"ReloadScripts", // name
			ui::menuItem,	// type
			_("Reload Scripts"),	// caption
			"",	// icon
			"ReloadScripts"); // event name

	SceneNodeBuffer::Instance().clear();
}

void ScriptingSystem::shutdownModule()
{
	rMessage() << getName() << "::shutdownModule called." << std::endl;

	_scriptMenu = ui::ScriptMenuPtr();

	ScriptWindow::destroy();

	// Clear the buffer so that nodes finally get destructed
	SceneNodeBuffer::Instance().clear();

	_scriptPath.clear();

	// Free all interfaces
	_interfaces.clear();

	_initialised = false;

	Py_Finalize();
}

} // namespace script
