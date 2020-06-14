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
	IEntitySettings& getSettings() override;

	/**
	 * Create an instance of the given entity at the given position, and return
	 * the Node containing the new entity.
	 *
	 * @returns: the scene::INodePtr referring to the new entity.
	 */
	IEntityNodePtr createEntityFromSelection(const std::string& name, const Vector3& origin) override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
	virtual void shutdownModule() override;
};

} // namespace entity
