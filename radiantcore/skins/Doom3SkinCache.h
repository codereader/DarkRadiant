#pragma once

#include "Doom3ModelSkin.h"

#include <map>
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
    std::mutex _cacheLock;

	// List of all skins
	std::vector<std::string> _allSkins;

	// Map between model paths and a vector of names of the associated skins,
	// which are contained in the main NamedSkinMap.
    std::map<std::string, std::vector<std::string>> _modelSkins;

	sigc::signal<void> _sigSkinsReloaded;
    sigc::connection _declsReloadedConnection;
    sigc::connection _declCreatedConnection;
    sigc::connection _declRemovedConnection;
    sigc::connection _declRenamedConnection;

    // This instance monitors all existing skins for changes
    std::map<std::string, sigc::connection> _declChangedConnections;
    std::set<std::string> _skinsPendingReparse;

public:
    decl::ISkin::Ptr findSkin(const std::string& name) override;
    bool renameSkin(const std::string& oldName, const std::string& newName) override;
    decl::ISkin::Ptr copySkin(const std::string& nameOfOriginal, const std::string& nameOfCopy) override;
    const StringList& getSkinsForModel(const std::string& model) override;
    const StringList& getAllSkins() override;
    bool skinCanBeModified(const std::string& name) override;

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
    void updateModelsInScene();
    void onSkinDeclCreated(decl::Type type, const std::string& name);
    void onSkinDeclRemoved(decl::Type type, const std::string& name);
    void onSkinDeclRenamed(decl::Type type, const std::string& oldName, const std::string& newName);
    void onSkinDeclChanged(decl::ISkin& skin);

    void handleSkinAddition(const std::string& name); // requires held lock
    void handleSkinRemoval(const std::string& name); // requires held lock
    void subscribeToSkin(const decl::ISkin::Ptr& skin);
    void unsubscribeFromAllSkins();
    void ensureCacheIsUpdated(); // requires lock to be held
};

} // namespace
