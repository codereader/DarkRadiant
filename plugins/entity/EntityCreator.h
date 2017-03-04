#pragma once

#include "ientity.h"
#include "ieclass.h"

namespace entity
{

/// Implementation of the EntityCreator interface for Doom 3
class Doom3EntityCreator :
	public EntityCreator
{
public:

    // EntityCreator implementation
	IEntityNodePtr createEntity(const IEntityClassPtr& eclass) override;
	void connectEntities(const scene::INodePtr& source, const scene::INodePtr& target) override;
    ITargetManagerPtr createTargetManager() override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
	virtual void shutdownModule() override;
};
typedef std::shared_ptr<Doom3EntityCreator> Doom3EntityCreatorPtr;

} // namespace entity
