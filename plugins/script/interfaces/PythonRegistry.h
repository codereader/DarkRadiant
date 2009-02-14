#ifndef _PYTHON_REGISTRY_H_
#define _PYTHON_REGISTRY_H_

#include "iregistry.h"
#include <boost/python.hpp>

namespace script {

/**
 * greebo: This class provides the script interface for the GlobalRegistry module.
 */
class PythonRegistry
{
public:
	std::string get(const std::string& key) {
		return GlobalRegistry().get(key);
	}

	void set(const std::string& key, const std::string& value) {
		GlobalRegistry().set(key, value);
	}

	// Register this interface in the given namespace
	static void RegisterInterface(boost::python::object& nspace) {
		// Local wrapper instance
		static PythonRegistry _registry;

		// Add the module declaration to the given python namespace
		nspace["GlobalRegistry"] = boost::python::class_<PythonRegistry>("GlobalRegistry", boost::python::init<>())
			.def("get", &PythonRegistry::get)
			.def("set", &PythonRegistry::set)
		;

		// Now point the Python variable "GlobalRegistry" to the wrapper instance
		nspace["GlobalRegistry"] = boost::python::ptr(&_registry);
	}
};

} // namespace script

#endif /* _PYTHON_REGISTRY_H_ */
