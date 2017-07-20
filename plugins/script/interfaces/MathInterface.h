#pragma once

#include "iscript.h"

namespace script 
{

// ========== Math objects ==========

class MathInterface :
	public IScriptInterface
{
public:
	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
