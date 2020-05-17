#pragma once

#include "ientity.h"
#include "ieclass.h"

namespace entity
{

/// Implementation of the IEntityModule interface for Doom 3
class Doom3EntityModule :
	public IEntityModule
{
public:
    // EntityCreator implementation
	IEntityNodePtr createEntity(const IEntityClassPtr& eclass) override;
    ITargetManagerPtr createTargetManager() override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
	virtual void shutdownModule() override;
};

} // namespace entity
