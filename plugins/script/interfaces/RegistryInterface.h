#pragma once

#include "iregistry.h"
#include "iscript.h"
#include <pybind11/pybind11.h>

namespace script
{

/**
 * greebo: This class provides the script interface for the GlobalRegistry module.
 */
class RegistryInterface :
	public IScriptInterface
{
public:
	std::string get(const std::string& key)
	{
		return GlobalRegistry().get(key);
	}

	void set(const std::string& key, const std::string& value)
	{
		GlobalRegistry().set(key, value);
	}

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override
	{
		// Add the module declaration to the given python namespace
		py::class_<RegistryInterface> registry(scope, "Registry");
		registry.def("get", &RegistryInterface::get);
		registry.def("set", &RegistryInterface::set);

		// Now point the Python variable "GlobalRegistry" to this instance
		globals["GlobalRegistry"] = this;
	}
};

} // namespace script
