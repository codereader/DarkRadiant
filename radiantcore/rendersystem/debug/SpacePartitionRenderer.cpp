#include "SpacePartitionRenderer.h"

#include "registry/adaptors.h"
#include "module/StaticModule.h"
#include <functional>

namespace render
{

namespace
{
	const std::string RKEY_RENDER_SPACE_PARTITION = "debug/ui/scenegraph/renderSpacePartition";
}

void SpacePartitionRenderer::toggle(const cmd::ArgumentList& args)
{
    registry::setValue(
		RKEY_RENDER_SPACE_PARTITION,
		!registry::getValue<bool>(RKEY_RENDER_SPACE_PARTITION)
	);
}

const std::string& SpacePartitionRenderer::getName() const
{
	static std::string _name("SpacePartitionRenderer");
	return _name;
}

const StringSet& SpacePartitionRenderer::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_SCENEGRAPH);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void SpacePartitionRenderer::initialiseModule(const IApplicationContext& ctx)
{
    registry::observeBooleanKey(
        RKEY_RENDER_SPACE_PARTITION,
        sigc::mem_fun(this, &SpacePartitionRenderer::installRenderer),
        sigc::mem_fun(this, &SpacePartitionRenderer::uninstallRenderer)
    );

	if (registry::getValue<bool>(RKEY_RENDER_SPACE_PARTITION))
	{
		installRenderer();
	}

	// Add the icon to the toolbar
	GlobalCommandSystem().addCommand("ToggleSpacePartitionRendering", std::bind(&SpacePartitionRenderer::toggle, this, std::placeholders::_1));
}

void SpacePartitionRenderer::shutdownModule()
{
	if (registry::getValue<bool>(RKEY_RENDER_SPACE_PARTITION))
	{
		uninstallRenderer();
	}
}

void SpacePartitionRenderer::installRenderer()
{
	_renderableSP.setSpacePartition(GlobalSceneGraph().getSpacePartition());
	_renderableSP.setRenderSystem(std::dynamic_pointer_cast<RenderSystem>(
		module::GlobalModuleRegistry().getModule(MODULE_RENDERSYSTEM)));

	GlobalRenderSystem().attachRenderable(_renderableSP);
}

void SpacePartitionRenderer::uninstallRenderer()
{
	_renderableSP.setRenderSystem(RenderSystemPtr());
	_renderableSP.setSpacePartition(scene::ISpacePartitionSystemPtr());

	GlobalRenderSystem().detachRenderable(_renderableSP);
}

#ifdef _DEBUG
// The module is only active in debug builds
module::StaticModuleRegistration<SpacePartitionRenderer> spacePartitionModule;
#endif

} // namespace render
