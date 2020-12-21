#pragma once

#include <memory>
#include <vector>
#include <utility>
#include <pybind11/pybind11.h>
namespace py = pybind11;

namespace script
{

class IScriptInterface
{
public:
    using Ptr = std::shared_ptr<IScriptInterface>;

    virtual ~IScriptInterface() {}

	/**
	* This method is called by the Scripting System to let this class
	* add its objects to the Python context.
	*/
	virtual void registerInterface(py::module& scope, py::dict& globals) = 0;
};

using NamedInterface = std::pair<std::string, IScriptInterface::Ptr>;
using NamedInterfaces = std::vector<NamedInterface>;

}
