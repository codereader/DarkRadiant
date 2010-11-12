#ifndef _SPACE_PARTITION_RENDERER_H_
#define _SPACE_PARTITION_RENDERER_H_

#include "iregistry.h"
#include "icommandsystem.h"
#include "iscenegraph.h"

#include "render/RenderableSpacePartition.h"

namespace render
{

class SpacePartitionRenderer :
	public RegistryKeyObserver,
	public RegisterableModule
{
	RenderableSpacePartition _renderableSP;

public:
	// RegistryKeyObserver implementation
	void keyChanged(const std::string& changedKey, const std::string& newValue);

	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	void toggle(const cmd::ArgumentList& args);

private:
	void installRenderer();
	void uninstallRenderer();
};

} // namespace render

#endif /* _SPACE_PARTITION_RENDERER_H_ */
