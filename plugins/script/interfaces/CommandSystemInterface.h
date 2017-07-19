#pragma once

#include "iscript.h"

namespace script
{

class CommandSystemInterface :
	public IScriptInterface
{
public:
	void execute(const std::string& buffer);
	void addStatement(const std::string& statementName, const std::string& string);
	void removeCommand(const std::string& name);

	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
