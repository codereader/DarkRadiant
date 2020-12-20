#include "PythonModule.h"

// Stringify and concatenate the PYBIND11_VERSION_xxx preprocessor symbols
#define Str(x) #x
#define QUOTE(x) Str(x)
#define PYBIND11_VERSION_STR (QUOTE(PYBIND11_VERSION_MAJOR) "." QUOTE(PYBIND11_VERSION_MINOR) "." QUOTE(PYBIND11_VERSION_PATCH))

#include "itextstream.h"

namespace script
{

PythonModule::PythonModule(const NamedInterfaces& interfaceList) :
    _namedInterfaces(interfaceList)
{}

void PythonModule::Construct(const NamedInterfaces& interfaceList)
{
    _instance.reset(new PythonModule(interfaceList));

    rMessage() << "Registering darkradiant module to Python using pybind11 version " <<
        PYBIND11_VERSION_STR << std::endl;

    // Register the darkradiant module to Python
    int result = PyImport_AppendInittab(NAME(), InitModule);

    if (result == -1)
    {
        rError() << "Could not initialise Python module" << std::endl;
        return;
    }
}

const char* PythonModule::NAME()
{
	return "darkradiant";
}

py::module& PythonModule::GetModule()
{
    return _instance->_module;
}

py::dict& PythonModule::GetGlobals()
{
    return _instance->_globals;
}

void PythonModule::Destroy()
{
    _instance.reset();
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
		// Work through the known list of interfaces
		if (!_instance)
		{
            throw new std::runtime_error("PythonModule not instantiated, call PythonModule::Construct first");
		}

#if (PYBIND11_VERSION_MAJOR > 2) || (PYBIND11_VERSION_MAJOR == 2 && PYBIND11_VERSION_MINOR >= 6)
        // pybind11 2.6+ have deprecated the py::module constructors, use py::module::create_extension_module
        _instance->_module = py::module::create_extension_module(NAME(), "DarkRadiant Main Module", &_moduleDef);
#else
        _instance->_module = py::module(NAME());
#endif

        // Add the registered interfaces
        for (const auto& i : _instance->_namedInterfaces)
        {
            // Handle each interface in its own try/catch block
            try
            {
                i.second->registerInterface(GetModule(), GetGlobals());
            }
            catch (const py::error_already_set& ex)
            {
                rError() << "Error while initialising interface " << i.first << ": " << std::endl;
                rError() << ex.what() << std::endl;
            }
        }

		auto main = py::module::import("__main__");
		auto globals = main.attr("__dict__").cast<py::dict>();

		for (auto i = globals.begin(); i != globals.end(); ++i)
		{
			GetGlobals()[(*i).first] = (*i).second;
		}

		return GetModule().ptr();
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

std::unique_ptr<PythonModule> PythonModule::_instance;
py::module::module_def PythonModule::_moduleDef;

}
