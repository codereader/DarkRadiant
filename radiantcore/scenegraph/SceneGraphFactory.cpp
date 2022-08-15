#include "SceneGraphFactory.h"

#include "itextstream.h"
#include "SceneGraph.h"

namespace scene
{

GraphPtr SceneGraphFactory::createSceneGraph()
{
	return std::make_shared<SceneGraph>();
}

const std::string& SceneGraphFactory::getName() const
{
	static std::string _name(MODULE_SCENEGRAPHFACTORY);
	return _name;
}

const StringSet& SceneGraphFactory::getDependencies() const
{
	static StringSet _dependencies; // no dependencies
	return _dependencies;
}

void SceneGraphFactory::initialiseModule(const IApplicationContext& ctx)
{
}

} // namespace
