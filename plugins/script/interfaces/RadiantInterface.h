#pragma once

#include "iradiant.h"
#include "iscript.h"
#include "iscriptinterface.h"

#include "EntityInterface.h"

namespace script 
{

/**
 * greebo: This class registers the base set of interfaces, like
 * entity types, primitives, etc.
 */
class RadiantInterface :
	public IScriptInterface
{
public:
	// Returns the first entity matching the given classname
	ScriptEntityNode findEntityByClassname(const std::string& name);
    // Find the entity with the given name
	ScriptEntityNode findEntityByName(const std::string& name);

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
