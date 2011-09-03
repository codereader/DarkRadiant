#pragma once

#include "iscenegraphfactory.h"

namespace scene
{

class SceneGraphFactory :
	public ISceneGraphFactory
{
public:
	GraphPtr createSceneGraph();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<SceneGraphFactory> SceneGraphFactoryPtr;

} // namespace
