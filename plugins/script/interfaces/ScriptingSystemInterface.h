#pragma once

#include "iscriptinterface.h"
#include <pybind11/pybind11.h>

namespace script
{

class ScriptingSystem;

/**
 * Exposes the GlobalScriptingSystem interface to scripts
 */
class ScriptingSystemInterface :
    public IScriptInterface
{
private:
    ScriptingSystem& _scriptingSystem;

public:
    ScriptingSystemInterface(ScriptingSystem& scriptingSystem) :
        _scriptingSystem(scriptingSystem)
    {}

    void registerBuiltinScriptCommand(py::object& cls);

    // IScriptInterface implementation
    void registerInterface(py::module& scope, py::dict& globals) override;
};

}
