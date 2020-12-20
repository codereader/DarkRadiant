#pragma once

#include "iscript.h"
#include "iscriptinterface.h"
#include <functional>
#include <memory>
#include <pybind11/pybind11.h>

#include "PythonConsoleWriter.h"

namespace py = pybind11;

namespace script
{

class PythonModule final
{
private:
	// Python objects and initialisation stuff
	py::module _module;
	py::dict _globals;
    static py::module::module_def _moduleDef;

    // Reference to the list owned by the ScriptingSystem
	const NamedInterfaces& _namedInterfaces;

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

public:
    PythonModule(const NamedInterfaces& interfaceList);
    ~PythonModule();

    // Starts up the interpreter, imports the darkradiant module
    void initialise();

    ExecutionResultPtr executeString(const std::string& scriptString);

	static const char* NAME();

	// Get the module object
	py::module& getModule();

	// Get the globals
	py::dict& getGlobals();

private:
    // Register the darkradiant module with the inittab pointing to InitModule
    void registerModule();

	// Endpoint called by the Python interface to acquire the module
#if PY_MAJOR_VERSION >= 3
	static PyObject* InitModule();
#else
	static void InitModule();
#endif

	static PyObject* InitModuleImpl();
};

}
