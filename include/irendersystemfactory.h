#pragma once

#include "imodule.h"
#include "irender.h"

class RenderSystem;
typedef std::shared_ptr<RenderSystem> RenderSystemPtr;

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
	static module::InstanceReference<render::IRenderSystemFactory> _reference(MODULE_RENDERSYSTEMFACTORY);
	return _reference;
}
