#pragma once

#include "iscriptinterface.h"

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

    void registerBuiltinScriptCommand();

    // IScriptInterface implementation
    void registerInterface(py::module& scope, py::dict& globals) override;
};

}
