#pragma once

#include "scene/Entity.h"
#include "ieclass.h"
#include <sigc++/connection.h>

namespace entity
{

/// Implementation of the IEntityModule interface for Doom 3
class Doom3EntityModule final :
	public IEntityModule
{
private:
    sigc::connection _settingsListener;

public:
    // EntityCreator implementation
	EntityNodePtr createEntity(const IEntityClassPtr& eclass) override;
    ITargetManagerPtr createTargetManager() override;
	IEntitySettings& getSettings() override;

	/**
	 * Create an instance of the given entity at the given position, and return
	 * the Node containing the new entity.
	 *
	 * @returns: the scene::INodePtr referring to the new entity.
	 */
	EntityNodePtr createEntityFromSelection(const std::string& name, const Vector3& origin) override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
    void onEntitySettingsChanged();
};

} // namespace entity
