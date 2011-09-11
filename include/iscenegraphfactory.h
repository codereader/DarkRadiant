#pragma once

#include "iscenegraph.h"

namespace scene
{

/**
 * greebo: The scenegraph factory can be used to generate
 * new instances of DarkRadiant's main scene manager.
 *
 * A Scenegraph consists of [0..N] scene::INodes, forming
 * an acyclic graph. There is one main scenegraph in DarkRadiant
 * accessible through GlobalSceneGraph(), but it's possible to have
 * more than this one, used for preview scenes for example.
 */
class ISceneGraphFactory :
	public RegisterableModule
{
public:
	/**
	 * Instantiates a new scenegraph.
	 */
	virtual GraphPtr createSceneGraph() = 0;
};

} // namespace

const char* const MODULE_SCENEGRAPHFACTORY = "SceneGraphFactory";

// Global accessor to the rendersystem factory module
inline scene::ISceneGraphFactory& GlobalSceneGraphFactory()
{
	// Cache the reference locally
	static scene::ISceneGraphFactory& _instance(
		*boost::static_pointer_cast<scene::ISceneGraphFactory>(
			module::GlobalModuleRegistry().getModule(MODULE_SCENEGRAPHFACTORY)
		)
	);
	return _instance;
}
