#pragma once

#include "iscriptinterface.h"
#include <functional>
#include <memory>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace script
{

class PythonModule
{
private:
	// Python objects and initialisation stuff
	py::module _module;
	py::dict _globals;
    static py::module::module_def _moduleDef;

    // Reference to the list owned by the ScriptingSystem
	const NamedInterfaces& _namedInterfaces;

    static std::unique_ptr<PythonModule> _instance;

    PythonModule(const NamedInterfaces& interfaceList);
    PythonModule(const PythonModule& other) = delete;
    PythonModule& operator=(const PythonModule& other) = delete;

public:
	static const char* NAME();

	// Get the module object
	static py::module& GetModule();

	// Get the globals
	static py::dict& GetGlobals();

	// Destroys the module and the globals object, no more calls to GetModule() and GetGlobals() afterwards!
	static void Destroy();

	// Endpoint called by the Python interface to acquire the module
#if PY_MAJOR_VERSION >= 3
	static PyObject* InitModule();
#else
	static void InitModule();
#endif

    static void Construct(const NamedInterfaces& interfaceList);

private:
	static PyObject* InitModuleImpl();
};

}
