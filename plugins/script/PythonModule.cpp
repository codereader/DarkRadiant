#include "PythonModule.h"

#include "itextstream.h"

namespace script
{

const char* PythonModule::NAME()
{
	return "darkradiant";
}

py::module& PythonModule::GetModule()
{
	if (!_module)
	{
		_module.reset(new py::module(NAME()));
	}

	return *_module;
}

void PythonModule::RegisterToPython(const ModuleRegistrationCallback& callback)
{
	_registrationCallback = callback;

	// Register the darkradiant module to Python
	int result = PyImport_AppendInittab(NAME(), InitModule);

	if (result == -1)
	{
		rError() << "Could not initialise Python module" << std::endl;
		return;
	}
}

py::dict& PythonModule::GetGlobals()
{
	if (!_globals)
	{
		_globals.reset(new py::dict);
	}

	return *_globals;
}

void PythonModule::Clear()
{
	_module.reset();
	_globals.reset();
}

#if PY_MAJOR_VERSION >= 3
PyObject* PythonModule::InitModule()
{
	return InitModuleImpl();
}
#else
void PythonModule::InitModule()
{
	InitModuleImpl();
}
#endif

PyObject* PythonModule::InitModuleImpl()
{
	try
	{
		// Acquire modules here (through callback?)
		if (_registrationCallback)
		{
			_registrationCallback(GetModule(), GetGlobals());
		}

		py::object main = py::module::import("__main__");
		py::dict globals = main.attr("__dict__").cast<py::dict>();

		for (auto i = globals.begin(); i != globals.end(); ++i)
		{
			GetGlobals()[(*i).first] = (*i).second;
		}

		return _module->ptr();
	}
	catch (py::error_already_set& e)
	{
		//e.clear();
		PyErr_SetString(PyExc_ImportError, e.what());
		return nullptr;
	}
	catch (const std::exception& e)
	{
		PyErr_SetString(PyExc_ImportError, e.what());
		return nullptr;
	}
}

std::unique_ptr<py::module> PythonModule::_module;
std::unique_ptr<py::dict> PythonModule::_globals;
PythonModule::ModuleRegistrationCallback PythonModule::_registrationCallback;

}
