#include "ScriptingSystem.h"

#include <pybind11/pybind11.h>
#include <pybind11/attr.h>
#include <pybind11/eval.h>

#include "i18n.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ui/imainframe.h"
#include "iundo.h"

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
#include "interfaces/SelectionGroupInterface.h"
#include "interfaces/CameraInterface.h"
#include "interfaces/LayerInterface.h"
#include "interfaces/DeclarationManagerInterface.h"
#include "interfaces/FxManagerInterface.h"

#include "PythonModule.h"

#include "SceneNodeBuffer.h"

#include "os/fs.h"
#include "os/file.h"
#include "os/path.h"
#include <functional>
#include "string/case_conv.h"

namespace script 
{

ScriptingSystem::ScriptingSystem() :
	_initialised(false)
{}

void ScriptingSystem::addInterface(const std::string& name, const IScriptInterfacePtr& iface)
{
    _pythonModule->addInterface(NamedInterface(name, iface));
}

void ScriptingSystem::executeScriptFile(const std::string& filename)
{
	executeScriptFile(filename, false);
}

void ScriptingSystem::executeScriptFile(const std::string& filename, bool setExecuteCommandAttr)
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

		if (setExecuteCommandAttr)
		{
			locals["__executeCommand__"] = true;
		}

		// Attempt to run the specified script
		py::eval_file(filePath, _pythonModule->getGlobals(), locals);
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
    return _pythonModule->executeString(scriptString);
}

void ScriptingSystem::foreachScriptCommand(const std::function<void(const IScriptCommand&)>& functor)
{
	for (const auto& pair : _commands)
	{
		if (pair.first == "Example") continue; // skip the example script

		functor(*pair.second);
	}
}

sigc::signal<void>& ScriptingSystem::signal_onScriptsReloaded()
{
	return _sigScriptsReloaded;
}

void ScriptingSystem::initialise()
{
    // Fire up the interpreter
    _pythonModule->initialise();

	_initialised = true;

	// Start the init script
	executeScriptFile("init.py");

	// Search script folder for commands
	reloadScripts();
}

void ScriptingSystem::runScriptFile(const cmd::ArgumentList& args)
{
	if (args.empty()) return;

	executeScriptFile(args[0].getString());
}

void ScriptingSystem::runScriptCommand(const cmd::ArgumentList& args) 
{
	if (args.empty()) return;

	executeCommand(args[0].getString());
}

void ScriptingSystem::reloadScriptsCmd(const cmd::ArgumentList& args) 
{
	reloadScripts();
}

void ScriptingSystem::executeCommand(const std::string& name)
{
	// Sanity check
	if (!_initialised)
	{
		rError() << "Cannot execute script command " << name
			<< ", ScriptingSystem not initialised yet." << std::endl;
		return;
	}

	// Lookup the name
	ScriptCommandMap::iterator found = _commands.find(name);

	if (found == _commands.end())
	{
		rError() << "Couldn't find command " << name << std::endl;
		return;
	}

    UndoableCommand cmd("runScriptCommand " + name);

	// Execute the script file behind this command
	executeScriptFile(found->second->getFilename(), true);
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
		py::eval_file(_scriptPath + scriptFilename, _pythonModule->getGlobals(), locals);

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
			auto cmd = std::make_shared<ScriptCommand>(cmdName, cmdDisplayName, scriptFilename);

			// Try to register this named command
			auto result = _commands.insert(std::make_pair(cmdName, cmd));

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

	if (!fs::exists(start))
	{
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
		string::to_lower(extension);

		if (extension != "py") continue;

		// Script file found, construct a new command
		loadCommandScript(os::getRelativePath(candidate.generic_string(), _scriptPath));
	}

	rMessage() << "ScriptModule: Found " << _commands.size() << " commands." << std::endl;

	_sigScriptsReloaded.emit();
}

// RegisterableModule implementation
const std::string& ScriptingSystem::getName() const
{
	static std::string _name(MODULE_SCRIPTING_SYSTEM);
	return _name;
}

const StringSet& ScriptingSystem::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void ScriptingSystem::initialiseModule(const IApplicationContext& ctx)
{
	// Subscribe to get notified as soon as Radiant is fully initialised
	module::GlobalModuleRegistry().signal_allModulesInitialised()
		.connect(sigc::mem_fun(this, &ScriptingSystem::initialise));

	// Construct the script path
	_scriptPath = ctx.getRuntimeDataPath() + "scripts/";

	// Set up the python interpreter
    _pythonModule.reset(new PythonModule);

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
	addInterface("SelectionGroupInterface", std::make_shared<SelectionGroupInterface>());
	addInterface("CameraInterface", std::make_shared<CameraInterface>());
	addInterface("LayerInterface", std::make_shared<LayerInterface>());
	addInterface("DeclarationManager", std::make_shared<DeclarationManagerInterface>());
	addInterface("FxManager", std::make_shared<FxManagerInterface>());

	GlobalCommandSystem().addCommand(
		"RunScript",
		std::bind(&ScriptingSystem::runScriptFile, this, std::placeholders::_1),
		{ cmd::ARGTYPE_STRING }
	);

	GlobalCommandSystem().addCommand(
		"ReloadScripts",
		std::bind(&ScriptingSystem::reloadScriptsCmd, this, std::placeholders::_1)
	);

	GlobalCommandSystem().addCommand(
		"RunScriptCommand",
		std::bind(&ScriptingSystem::runScriptCommand, this, std::placeholders::_1),
		{ cmd::ARGTYPE_STRING }
	);

	SceneNodeBuffer::Instance().clear();
}

void ScriptingSystem::shutdownModule()
{
	rMessage() << getName() << "::shutdownModule called." << std::endl;

	_sigScriptsReloaded.clear();

	_initialised = false;

	// Clear the buffer so that nodes finally get destructed
	SceneNodeBuffer::Instance().clear();

	_commands.clear();

	_scriptPath.clear();

    _pythonModule.reset();
}

} // namespace script
