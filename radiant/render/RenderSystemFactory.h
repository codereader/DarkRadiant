#ifndef _RENDERSYSTEM_FACTORY_IMPL_H_
#define _RENDERSYSTEM_FACTORY_IMPL_H_

#include "irendersystemfactory.h"

namespace render 
{

class OpenGLRenderSystem;
typedef boost::shared_ptr<OpenGLRenderSystem> OpenGLRenderSystemPtr;

class RenderSystemFactory :
	public IRenderSystemFactory
{
private:
	// The default rendersystem instance
	OpenGLRenderSystemPtr _defaultRenderSystem;

public:
	// IRenderSystemFactory implementation
	RenderSystemPtr createRenderSystem();
	RenderSystem& getDefaultRenderSystem();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
};

} // namespace

#endif /* _RENDERSYSTEM_FACTORY_IMPL_H_ */
