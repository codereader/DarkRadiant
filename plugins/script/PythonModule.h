#pragma once

#include "iscript.h"
#include "iscriptinterface.h"
#include <functional>
#include <memory>
#include <pybind11/pybind11.h>

#include "PythonConsoleWriter.h"
#include "ScriptCommand.h"

namespace py = pybind11;

namespace script
{

class PythonModule final
{
private:
    std::unique_ptr<py::module::module_def> _moduleDef;

	// Python module and global dictionary
	py::module _module;
	std::unique_ptr<py::dict> _globals;

    // List of registered interfaces
	NamedInterfaces _namedInterfaces;

    PythonModule(const PythonModule& other) = delete;
    PythonModule& operator=(const PythonModule& other) = delete;

    // We need a static reference to the current object, since 
    // PyImport_AppendInittab doesn't allow us to pass user data
    static PythonModule* _instance;

    // Console and Error output handling
    std::string _outputBuffer;
    std::string _errorBuffer;

    PythonConsoleWriter _outputWriter;
    PythonConsoleWriter _errorWriter;

    bool _interpreterInitialised;

public:
    PythonModule();
    ~PythonModule();

    // Starts up the interpreter, imports the darkradiant module
    void initialise();

    ExecutionResultPtr executeString(const std::string& scriptString);

    // Execute the given script file
    void executeScriptFile(const std::string& scriptBasePath, const std::string& relativeScriptPath, bool setExecuteCommandAttr);

	// Get the global object dictionary of this module
	py::dict& getGlobals();

    void addInterface(const NamedInterface& iface);

    // Attempts to create a script command from the given .py file
    // Will return an empty object if the file path is not a valid file
    ScriptCommand::Ptr createScriptCommand(const std::string& scriptBasePath, const std::string& relativeScriptPath);

private:
    // Register the darkradiant module with the inittab pointing to InitModule
    void registerModule();

    bool interfaceExists(const std::string& name);

	// Endpoint called by the Python interface to acquire the module
#if PY_MAJOR_VERSION >= 3
	static PyObject* InitModule();
#else
	static void InitModule();
#endif

	PyObject* initialiseModule();
};

}
