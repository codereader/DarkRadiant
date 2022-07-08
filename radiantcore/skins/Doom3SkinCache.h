#pragma once

#include "Doom3ModelSkin.h"

#include "imodule.h"

#include "modelskin.h"

#include <sigc++/connection.h>
#include <map>
#include <string>
#include <vector>

namespace skins
{

/**
 * Implementation of ModelSkinCache interface for Doom 3 skin management.
 */
class Doom3SkinCache :
    public decl::IModelSkinCache
{
	// List of all skins
	std::vector<std::string> _allSkins;

	// Map between model paths and a vector of names of the associated skins,
	// which are contained in the main NamedSkinMap.
    std::map<std::string, std::vector<std::string>> _modelSkins;

	sigc::signal<void> _sigSkinsReloaded;
    sigc::connection _declsReloadedConnection;

public:
    decl::ISkin::Ptr findSkin(const std::string& name) override;
    const StringList& getSkinsForModel(const std::string& model) override;
    const StringList& getAllSkins() override;

	/**
	 * greebo: Clears and reloads all skins.
	 */
	void refresh() override;

	// Public events
	sigc::signal<void> signal_skinsReloaded() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    void onSkinDeclsReloaded();
};

} // namespace
