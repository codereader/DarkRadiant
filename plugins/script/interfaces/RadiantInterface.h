#ifndef _RADIANT_INTERFACE_H_
#define _RADIANT_INTERFACE_H_

#include <boost/python.hpp>

#include "iradiant.h"
#include "iscript.h"

#include "EntityInterface.h"

namespace script {

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

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<RadiantInterface> RadiantInterfacePtr;

} // namespace script

#endif /* _RADIANT_INTERFACE_H_ */
