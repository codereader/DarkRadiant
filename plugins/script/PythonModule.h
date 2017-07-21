#pragma once

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
	static std::unique_ptr<py::module> _module;
	static std::unique_ptr<py::dict> _globals;

	typedef std::function<void(py::module&, py::dict&)> ModuleRegistrationCallback;
	static ModuleRegistrationCallback _registrationCallback;

public:
	static const char* NAME();

	// Get the module object
	static py::module& GetModule();

	// Get the globals
	static py::dict& GetGlobals();

	static void RegisterToPython(const ModuleRegistrationCallback& callback);

	// Destroys the module and the globals object, no more calls to GetModule() and GetGlobals() afterwards!
	static void Clear();

	// Endpoint called by the Python interface to acquire the module
#if PY_MAJOR_VERSION >= 3
	static PyObject* InitModule();
#else
	static void InitModule();
#endif

private:
	static PyObject* InitModuleImpl();
};

}
