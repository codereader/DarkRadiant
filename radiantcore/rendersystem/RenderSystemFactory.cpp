#include "RenderSystemFactory.h"

#include "itextstream.h"
#include "module/StaticModule.h"

#include "OpenGLRenderSystem.h"

namespace render
{

RenderSystemPtr RenderSystemFactory::createRenderSystem()
{
    return std::make_shared<OpenGLRenderSystem>();
}

const std::string& RenderSystemFactory::getName() const
{
	static std::string _name(MODULE_RENDERSYSTEMFACTORY);
	return _name;
}

const StringSet& RenderSystemFactory::getDependencies() const
{
	static StringSet _dependencies; // no dependencies
	return _dependencies;
}

void RenderSystemFactory::initialiseModule(const IApplicationContext& ctx)
{
}

// Define the static RenderSystemFactory module
module::StaticModuleRegistration<RenderSystemFactory> renderSystemFactory;

} // namespace
