#include "SpacePartitionRenderer.h"

#include "modulesystem/StaticModule.h"
#include <boost/bind.hpp>

namespace render
{

namespace
{
	const std::string RKEY_RENDER_SPACE_PARTITION = "debug/ui/scenegraph/renderSpacePartition";
}

void SpacePartitionRenderer::keyChanged(const std::string& changedKey, const std::string& newValue)
{
	if (newValue == "1")
	{
		installRenderer();
	}
	else
	{
		uninstallRenderer();
	}
}

void SpacePartitionRenderer::toggle(const cmd::ArgumentList& args)
{
	GlobalRegistry().set(
		RKEY_RENDER_SPACE_PARTITION,
		GlobalRegistry().get(RKEY_RENDER_SPACE_PARTITION) != "1" ? "1" : "0"
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

void SpacePartitionRenderer::initialiseModule(const ApplicationContext& ctx)
{
	GlobalRegistry().addKeyObserver(this, RKEY_RENDER_SPACE_PARTITION);

	if (GlobalRegistry().get(RKEY_RENDER_SPACE_PARTITION) == "1")
	{
		installRenderer();
	}

	// Add the icon to the toolbar
	GlobalCommandSystem().addCommand("ToggleSpacePartitionRendering", boost::bind(&SpacePartitionRenderer::toggle, this, _1));
}

void SpacePartitionRenderer::shutdownModule()
{
	if (GlobalRegistry().get(RKEY_RENDER_SPACE_PARTITION) == "1")
	{
		uninstallRenderer();
	}

	GlobalRegistry().removeKeyObserver(this);
}

void SpacePartitionRenderer::installRenderer()
{
	_renderableSP.setSpacePartition(GlobalSceneGraph().getSpacePartition());
	_renderableSP.setShader(GlobalRenderSystem().capture("[1 0 0]"));

	GlobalRenderSystem().attachRenderable(_renderableSP);
}

void SpacePartitionRenderer::uninstallRenderer()
{
	_renderableSP.setShader(ShaderPtr());
	_renderableSP.setSpacePartition(scene::ISpacePartitionSystemPtr());

	GlobalRenderSystem().detachRenderable(_renderableSP);
}

#ifdef _DEBUG
// The module is only active in debug builds
module::StaticModule<SpacePartitionRenderer> spacePartitionModule;
#endif

} // namespace render
