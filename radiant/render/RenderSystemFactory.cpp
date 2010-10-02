#include "RenderSystemFactory.h"

#include "itextstream.h"
#include "modulesystem/StaticModule.h"

#include "OpenGLRenderSystem.h"

namespace render
{

RenderSystemPtr RenderSystemFactory::createRenderSystem()
{
	throw std::runtime_error("not implemented.");
}

RenderSystem& RenderSystemFactory::getDefaultRenderSystem()
{
	if (!_defaultRenderSystem)
	{
		// Create the default rendersystem on demand
		_defaultRenderSystem.reset(new OpenGLRenderSystem);
		_defaultRenderSystem->capture("$OVERBRIGHT");
	}

	return *_defaultRenderSystem;
}

const std::string& RenderSystemFactory::getName() const
{
	static std::string _name(MODULE_RENDERSYSTEMFACTORY);
	return _name;
}

const StringSet& RenderSystemFactory::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_SHADERSYSTEM);
		_dependencies.insert(MODULE_OPENGL);
	}

	return _dependencies;
}

void RenderSystemFactory::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << getName() << "::initialiseModule called." << std::endl;
}

void RenderSystemFactory::shutdownModule()
{
	_defaultRenderSystem.reset();
}

// Define the static RenderSystemFactory module
module::StaticModule<RenderSystemFactory> renderSystemFactory;

} // namespace
