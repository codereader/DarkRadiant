#ifndef _MAP_INTERFACE_H_
#define _MAP_INTERFACE_H_

#include <boost/python.hpp>

#include "iscript.h"

#include "SceneGraphInterface.h"

namespace script {

/**
 * greebo: This class provides the script interface for the GlobalEntityClassManager module.
 */
class MapInterface :
	public IScriptInterface
{
public:
	ScriptSceneNode getWorldSpawn();
	std::string getMapName();

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<MapInterface> MapInterfacePtr;

} // namespace script

#endif /* _MAP_INTERFACE_H_ */
