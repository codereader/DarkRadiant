#pragma once

#include "ieclass.h"
#include "iscript.h"
#include "iscriptinterface.h"

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

	operator const IEntityClassPtr&() const
	{
		return _eclass;
	}

    // Returns a specific spawnarg value from this entityDef, or "" if not found
    std::string getAttributeValue(const std::string& name)
    {
        if (!_eclass)
            return {};
        else
            return _eclass->getAttributeValue(name);
    }

    bool isNull() const
	{
		return !_eclass;
	}

	bool isOfType(const std::string& className)
	{
		return !_eclass ? false : _eclass->isOfType(className);
	}

	std::string getDefFileName()
	{
		return _eclass ? _eclass->getDefFileName() : std::string();
	}
};

// Wrap around the EntityClassVisitor interface
class EntityClassVisitorWrapper :
	public EntityClassVisitor
{
public:
    void visit(const IEntityClassPtr& eclass) override
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
    void visit(const IModelDefPtr& modelDef) override
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
