#include "ScriptingSystem.h"

#include "itextstream.h"
#include "SysObject.h"

#include <boost/python.hpp>

namespace script {

ScriptingSystem::ScriptingSystem() :
	_outputWriter(false),
	_errorWriter(true)
{}

// RegisterableModule implementation
const std::string& ScriptingSystem::getName() const {
	static std::string _name("ScriptingSystem");
	return _name;
}

const StringSet& ScriptingSystem::getDependencies() const {
	static StringSet _dependencies; 
	return _dependencies;
}

void ScriptingSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << getName() << "::initialiseModule called.\n";

	// start the python interpreter
	Py_Initialize();

	globalOutputStream() << getName() << ": Python interpreter initialised.\n";

	boost::python::object main_module = boost::python::import("__main__");
	boost::python::object main_namespace = main_module.attr("__dict__");
	boost::python::dict globals;

	std::string appPath = ctx.getApplicationPath() + "scripts/";

	try {
		// Declare the console writer to Python
		main_namespace["PythonConsoleWriter"] = PythonConsoleWriterClass("PythonConsoleWriter", boost::python::init<bool>())
			.def("write", &PythonConsoleWriter::write);
		
		// Redirect stdio output to our local instances
		boost::python::import("sys").attr("stderr") = _errorWriter;
		boost::python::import("sys").attr("stdout") = _outputWriter; 

		// Declare the Temp object in python
		main_namespace["Temp"] = boost::python::class_<SysObject>("Temp")
			.def("printToConsole", &SysObject::print);

		boost::python::object ignored = boost::python::exec_file(
			(appPath + "init.py").c_str(),
			main_namespace,
			globals
		);
	}
	catch (const boost::python::error_already_set& e) {
		// Dump the error to the console, this will invoke the PythonConsoleWriter
		PyErr_Print();
		PyErr_Clear();
	}
}

} // namespace script
