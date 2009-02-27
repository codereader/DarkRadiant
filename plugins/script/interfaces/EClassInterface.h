#ifndef _ECLASS_INTERFACE_H_
#define _ECLASS_INTERFACE_H_

#include <boost/python.hpp>

#include "ieclass.h"
#include "iscript.h"

namespace script {

/**
 * This class represents a single EntityDef / EntityClass for script.
 */
class ScriptEntityClass
{
	IEntityClassConstPtr _eclass;
	EntityClassAttribute _emptyAttribute;

public:
	ScriptEntityClass(const IEntityClassConstPtr& eclass) :
		_eclass(eclass)
	{}

	operator const IEntityClassConstPtr&() const {
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
};

// Wrap around the EntityClassVisitor interface
class EntityClassVisitorWrapper : 
	public EntityClassVisitor, 
	public boost::python::wrapper<EntityClassVisitor>
{
public:
    void visit(IEntityClassPtr eclass) {
		// Wrap this method to python
		this->get_override("visit")(ScriptEntityClass(eclass));
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

	void forEach(EntityClassVisitor& visitor);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<EClassManagerInterface> EClassManagerInterfacePtr;

} // namespace script

#endif /* _ECLASS_INTERFACE_H_ */
