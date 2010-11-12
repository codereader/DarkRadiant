#ifndef _IRENDERSYSTEM_FACTORY_H_
#define _IRENDERSYSTEM_FACTORY_H_

#include "imodule.h"
#include "irender.h"

class RenderSystem;
typedef boost::shared_ptr<RenderSystem> RenderSystemPtr;

namespace render
{

/**
 * greebo: The rendersystem factory can be used to generate
 * new instances of DarkRadiant's backend renderer.
 *
 * The backend renderer provides access to named Shader objects
 * which can be filled with OpenGLRenderable objects.
 */
class IRenderSystemFactory :
	public RegisterableModule
{
public:
	/**
	 * Instantiates a new rendersystem.
	 */
	virtual RenderSystemPtr createRenderSystem() = 0;
};

} // namespace

const char* const MODULE_RENDERSYSTEMFACTORY = "RenderSystemFactory";

// Global accessor to the rendersystem factory module
inline render::IRenderSystemFactory& GlobalRenderSystemFactory()
{
	// Cache the reference locally
	static render::IRenderSystemFactory& _instance(
		*boost::static_pointer_cast<render::IRenderSystemFactory>(
			module::GlobalModuleRegistry().getModule(MODULE_RENDERSYSTEMFACTORY)
		)
	);
	return _instance;
}

#endif /* _IRENDERSYSTEM_FACTORY_H_ */
