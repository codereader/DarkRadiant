#ifndef _REGISTRY_INTERFACE_H_
#define _REGISTRY_INTERFACE_H_

#include "iregistry.h"
#include "iscript.h"
#include <boost/python.hpp>

namespace script {

/**
 * greebo: This class provides the script interface for the GlobalRegistry module.
 */
class RegistryInterface :
	public IScriptInterface
{
public:
	std::string get(const std::string& key) {
		return GlobalRegistry().get(key);
	}

	void set(const std::string& key, const std::string& value) {
		GlobalRegistry().set(key, value);
	}

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace) {
		// Add the module declaration to the given python namespace
		nspace["GlobalRegistry"] = boost::python::class_<RegistryInterface>("GlobalRegistry")
			.def("get", &RegistryInterface::get)
			.def("set", &RegistryInterface::set)
		;

		// Now point the Python variable "GlobalRegistry" to this instance
		nspace["GlobalRegistry"] = boost::python::ptr(this);
	}
};
typedef boost::shared_ptr<RegistryInterface> RegistryInterfacePtr;

} // namespace script

#endif /* _REGISTRY_INTERFACE_H_ */
