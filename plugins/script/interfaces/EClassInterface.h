#pragma once

#include "ieclass.h"
#include "iscript.h"

#include <pybind11/pybind11.h>

namespace script 
{

/**
 * This class represents a single EntityDef / EntityClass for script.
 */
class ScriptEntityClass
{
	IEntityClassPtr _eclass;
	EntityClassAttribute _emptyAttribute;

public:
	ScriptEntityClass(const IEntityClassPtr& eclass) :
		_eclass(eclass),
		_emptyAttribute("text", "", "")
	{}

	operator const IEntityClassPtr&() const {
		return _eclass;
	}

	// Returns a specific spawnarg from this entityDef, or "" if not found
	const EntityClassAttribute& getAttribute(const std::string& name) {
		if (_eclass == NULL) {
			return _emptyAttribute;
		}

		return _eclass->getAttribute(name);
	}

	bool isNull() const {
		return _eclass == NULL;
	}

	bool isOfType(const std::string& className)
	{
		return _eclass == NULL ? false : _eclass->isOfType(className);
	}
};

// Wrap around the EntityClassVisitor interface
class EntityClassVisitorWrapper :
	public EntityClassVisitor
{
public:
    void visit(const IEntityClassPtr& eclass)
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,					/* Return type */
			EntityClassVisitor,		/* Parent class */
			visit,					/* Name of function in C++ (must match Python name) */
			ScriptEntityClass(eclass) /* Argument(s) */
		);
	}
};

// Wrap around the ModelDefVisitor interface
class ModelDefVisitorWrapper :
	public ModelDefVisitor
{
public:
    void visit(const IModelDefPtr& modelDef) 
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,				/* Return type */
			ModelDefVisitor,	/* Parent class */
			visit,				/* Name of function in C++ (must match Python name) */
			*modelDef			/* Argument(s) */
		);
	}
};

/**
 * greebo: This class provides the script interface for the GlobalEntityClassManager module.
 */
class EClassManagerInterface :
	public IScriptInterface
{
	IModelDef _emptyModelDef;

public:
	ScriptEntityClass findClass(const std::string& name);

	IModelDef findModel(const std::string& name);

	void forEachEntityClass(EntityClassVisitor& visitor);
	void forEachModelDef(ModelDefVisitor& visitor);

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
