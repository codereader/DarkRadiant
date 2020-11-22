#pragma once

#include <memory>
#include <pybind11/pybind11.h>
namespace py = pybind11;

namespace script
{

class IScriptInterface
{
public:
    virtual ~IScriptInterface() {}

	/**
	* This method is called by the Scripting System to let this class
	* add its objects to the Python context.
	*/
	virtual void registerInterface(py::module& scope, py::dict& globals) = 0;
};
typedef std::shared_ptr<IScriptInterface> IScriptInterfacePtr;

}
