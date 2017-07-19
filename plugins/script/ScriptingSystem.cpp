#include "ScriptingSystem.h"

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/attr.h>

#include "i18n.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "imainframe.h"
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

#include <wx/frame.h>

#include "os/fs.h"
#include "os/file.h"
#include "os/path.h"
#include <functional>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

class DarkRadiantModule
{
private:
	static std::unique_ptr<py::module> _module;
	static std::unique_ptr<py::dict> _globals;
	static std::function<void(py::module&, py::dict&)> _registrationCallback;

public:
	static py::module& GetModule()
	{
		if (!_module)
		{
			_module.reset(new py::module("darkradiant"));
		}

		return *_module;
	}

	static void SetRegistrationCallback(const std::function<void(py::module&, py::dict&)>& func)
	{
		_registrationCallback = func;
	}

	static py::dict& GetGlobals()
	{
		if (!_globals)
		{
			_globals.reset(new py::dict);
		}

		return *_globals;
	}

	static void Clear()
	{
		_module.reset();
		_globals.reset();
	}

	// Endpoint called by the Python interface to acquire the module
	static PyObject* pybind11_init_wrapper()
	{
		try
		{
			// Acquire modules here (through callback?)
			if (_registrationCallback)
			{
				_registrationCallback(GetModule(), GetGlobals());
			}

            return _module->ptr();
        } 
		catch (py::error_already_set& e)
		{                            
            e.clear();                                                        
            PyErr_SetString(PyExc_ImportError, e.what());                     
            return nullptr;                                                   
        } 
		catch (const std::exception& e)
		{                                   
            PyErr_SetString(PyExc_ImportError, e.what());                     
            return nullptr;                                                   
        }
	}
};

std::unique_ptr<py::module> DarkRadiantModule::_module;
std::unique_ptr<py::dict> DarkRadiantModule::_globals;
std::function<void(py::module&, py::dict&)> DarkRadiantModule::_registrationCallback;

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

	if (_initialised) 
	{
		// Add the interface at once, all the others are already added
		iface->registerInterface(_mainObjects->mainNamespace);

		iface->registerInterface(DarkRadiantModule::GetModule(), DarkRadiantModule::GetGlobals());
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

void ScriptingSystem::executeScriptFile(const std::string& filename) 
{
	try
	{
        std::string filePath = _scriptPath + filename;

        // Prevent calling exec_file with a non-existent file, we would
        // get crashes during Py_Finalize later on.
        if (!os::fileOrDirExists(filePath))
        { 
            rError() << "Error: File " << filePath << " doesn't exist." << std::endl;
            return;
        }

		py::dict locals;
		locals["__executeCommand__"] = true;

		// Attempt to run the specified script
		py::eval_file(filePath, py::globals(), locals);

#if 0
		boost::python::object ignored = boost::python::exec_file(
            filePath.c_str(),
			_mainObjects->mainNamespace,
			_mainObjects->globals
		);
#endif
	}
    catch (std::invalid_argument& e)
    {
        rError() << "Error trying to execute file " << filename << ": " << e.what() << std::endl;
    }
	catch (const py::error_already_set& ex)
	{
		rError() << "Error while executing file: " << filename << ": " << std::endl;
		rError() << ex.what() << std::endl;
	}
}

ExecutionResultPtr ScriptingSystem::executeString(const std::string& scriptString)
{
	ExecutionResultPtr result = std::make_shared<ExecutionResult>();

	result->errorOccurred = false;

	// Clear the output buffers before starting to execute
	_outputBuffer.clear();
	_errorBuffer.clear();

	try
	{
		// Attempt to run the specified script
		py::exec(scriptString);
	}
	catch (py::error_already_set& ex)
	{
		_errorBuffer.append(ex.what());
		result->errorOccurred = true;

		rError() << "Error executing script: " << ex.what() << std::endl;
	}

	result->output += _outputBuffer + "\n";
	result->output += _errorBuffer + "\n";

	_outputBuffer.clear();
	_errorBuffer.clear();

	return result;
}

void ScriptingSystem::addInterfacesToModule(py::module& mod, py::dict& globals)
{
	// Add the registered interfaces
	for (NamedInterface& i : _interfaces)
	{
		// Handle each interface in its own try/catch block
		try
		{
			i.second->registerInterface(mod, globals);
		}
		catch (const py::error_already_set& ex)
		{
			rError() << "Error while initialising interface " << i.first << ": " << std::endl;
			rError() << ex.what() << std::endl;
		}
	}
}

void ScriptingSystem::initialise()
{
	py::initialize_interpreter();

	{
		try
		{
			// Construct the console writer interface
			PythonConsoleWriterClass consoleWriter(DarkRadiantModule::GetModule(), "PythonConsoleWriter");
			consoleWriter.def(py::init<bool, std::string&>());
			consoleWriter.def("write", &PythonConsoleWriter::write);

			// Redirect stdio output to our local ConsoleWriter instances
			py::module::import("sys").attr("stderr") = &_errorWriter;
			py::module::import("sys").attr("stdout") = &_outputWriter;

			//py::exec("import darkradiant as dr\nfrom darkradiant import *\nGlobalCommandSystem.execute(Tork)\nprint('This is a test')", copy);
		}
		catch (const py::error_already_set& ex)
		{
			rError() << ex.what() << std::endl;
		}
	}

#if 0
	// Add the registered interfaces
	try {
		for (Interfaces::iterator i = _interfaces.begin(); i != _interfaces.end(); ++i) {
			// Handle each interface in its own try/catch block
			try
			{
				i->second->registerInterface(_mainObjects->mainNamespace);
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
#endif

	_initialised = true;

	// Start the init script
	executeScriptFile("init.py");

	// Search script folder for commands
	reloadScripts();

	// Add the scripting widget to the groupdialog
	IGroupDialog::PagePtr page(new IGroupDialog::Page);

	page->name = "ScriptWindow";
	page->windowLabel = _("Script");
	page->page = new ScriptWindow(GlobalMainFrame().getWxTopLevelWindow());
	page->tabIcon = "icon_script.png";
	page->tabLabel = _("Script");
	page->position = IGroupDialog::Page::Position::Console - 10; // insert before console

	GlobalGroupDialog().addPage(page);
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

#if 0
	// Set the execution flag in the global namespace
	_mainObjects->globals["__executeCommand__"] = true;
#endif

	// Execute the script file behind this command
	executeScriptFile(found->second->getFilename());
}

void ScriptingSystem::loadCommandScript(const std::string& scriptFilename)
{
	try
	{
		// Create a new dictionary for the initialisation routine
		py::dict locals;

		// Disable the flag for initialisation, just for sure
		locals["__executeCommand__"] = false;

		// Attempt to run the specified script
		py::eval_file((_scriptPath + scriptFilename), py::globals(), locals);

#if 0
		boost::python::object ignored = boost::python::exec_file(
			(_scriptPath + scriptFilename).c_str(),
			_mainObjects->mainNamespace,
			locals	// pass the new dictionary for the locals
		);
#endif

		std::string cmdName;
		std::string cmdDisplayName;

		if (locals.contains("__commandName__"))
		{
			cmdName = locals["__commandName__"].cast<std::string>();
		}

		if (locals.contains("__commandDisplayName__"))
		{
			cmdDisplayName = locals["__commandDisplayName__"].cast<std::string>();
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
			if (result.second) 
			{
				rMessage() << "Registered script file " << scriptFilename
					<< " as " << cmdName << std::endl;
			}
			else 
			{
				rError() << "Error in " << scriptFilename << ": Script command "
					<< cmdName << " has already been registered in "
					<< _commands[cmdName]->getFilename() << std::endl;
			}
		}
	}
	catch (const py::error_already_set& ex)
	{
		rError() << "Script file " << scriptFilename << " is not a valid command:" << std::endl;
		rError() << ex.what() << std::endl;
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
		loadCommandScript(os::getRelativePath(candidate.generic_string(), _scriptPath));
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
	_scriptPath = ctx.getRuntimeDataPath() + "scripts/";
#endif

	// When Python asks for the object, let's register our interfaces to the py::module
	DarkRadiantModule::SetRegistrationCallback(
		std::bind(&ScriptingSystem::addInterfacesToModule, this, std::placeholders::_1, std::placeholders::_2));

	// Register the darkradiant module to Python
	int result = PyImport_AppendInittab("darkradiant", DarkRadiantModule::pybind11_init_wrapper);

	if (result == -1)
	{
		rError() << "Could not initialise Python module" << std::endl;
		return;
	}

	// Add the built-in interfaces (the order is important, as we don't have dependency-resolution yet)
	addInterface("Math", std::make_shared<MathInterface>());
	addInterface("GameManager", std::make_shared<GameInterface>());
	addInterface("CommandSystem", std::make_shared<CommandSystemInterface>());
	addInterface("SceneGraph", std::make_shared<SceneGraphInterface>());
	addInterface("GlobalRegistry", std::make_shared<RegistryInterface>());
	addInterface("GlobalEntityClassManager", std::make_shared<EClassManagerInterface>());
	addInterface("GlobalSelectionSystem", std::make_shared<SelectionInterface>());
	addInterface("Brush", std::make_shared<BrushInterface>());
	addInterface("Patch", std::make_shared<PatchInterface>());
	addInterface("Entity", std::make_shared<EntityInterface>());
	addInterface("Radiant", std::make_shared<RadiantInterface>());
	addInterface("Map", std::make_shared<MapInterface>());
	addInterface("FileSystem", std::make_shared<FileSystemInterface>());
	addInterface("Grid", std::make_shared<GridInterface>());
	addInterface("ShaderSystem", std::make_shared<ShaderSystemInterface>());
	addInterface("Model", std::make_shared<ModelInterface>());
	addInterface("ModelSkinCacheInterface", std::make_shared<ModelSkinCacheInterface>());
	addInterface("SoundManager", std::make_shared<SoundManagerInterface>());
	addInterface("DialogInterface", std::make_shared<DialogManagerInterface>());
	addInterface("SelectionSetInterface", std::make_shared<SelectionSetInterface>());

#if 0
	// start the python interpreter
	Py_Initialize();

	rMessage() << getName() << ": Python interpreter initialised." << std::endl;

	_mainObjects.reset(new BoostPythonMainObjects);

	_mainObjects->mainModule = boost::python::import("__main__");
	_mainObjects->mainNamespace = _mainObjects->mainModule.attr("__dict__");

	try {
		// Construct the console writer interface
		PythonConsoleWriterClass consoleWriter("PythonConsoleWriter", boost::python::init<bool, std::string&>());
		consoleWriter.def("write", &PythonConsoleWriter::write);

		// Declare the interface to python
		_mainObjects->mainNamespace["PythonConsoleWriter"] = consoleWriter;

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
	_mainObjects->mainNamespace["StringVector"] = boost::python::class_< std::vector<std::string> >("StringVector")
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
#endif

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

#if 0
	// Clear the buffer so that nodes finally get destructed
	SceneNodeBuffer::Instance().clear();
    
    _commands.clear();

	_scriptPath.clear();

	// Free all interfaces
	_interfaces.clear();

	_initialised = false;

	// Clear the boost::python::objects
	_mainObjects.reset();

	Py_Finalize();
#endif

	_initialised = false;

	// Clear the buffer so that nodes finally get destructed
	SceneNodeBuffer::Instance().clear();

	_commands.clear();

	_scriptPath.clear();

	// Free all interfaces
	_interfaces.clear();

	DarkRadiantModule::Clear();

	py::finalize_interpreter();
}

} // namespace script
