#include "FavouritesManager.h"

#include "iregistry.h"
#include "idecltypes.h"
#include "module/StaticModule.h"

namespace game
{

namespace
{
    constexpr const char* const RKEY_MEDIABROWSER_LEGACY_ROOT = "user/ui/mediaBrowser/favourites";
    constexpr const char* const RKEY_FAVOURITES_ROOT = "user/ui/favourites";
    constexpr const char* const RKEY_LEGACY_SUBPATH_MATERIALS = "/materials";
    constexpr const char* const RKEY_LEGACY_SUBPATH_ENTITYDEFS = "/entityDefs";
    constexpr const char* const RKEY_LEGACY_SUBPATH_SOUNDSHADERS = "/soundShaders";
    constexpr const char* const RKEY_LEGACY_SUBPATH_MODELS = "/models";
    constexpr const char* const RKEY_LEGACY_SUBPATH_PARTICLES = "/particles";
}

void FavouritesManager::addFavourite(const std::string& typeName, const std::string& identifier)
{
    if (typeName.empty() || identifier.empty()) return;

    auto set = _favouritesByType.find(typeName);

    if (set == _favouritesByType.end())
    {
        set = _favouritesByType.emplace(typeName, FavouriteSet(typeName)).first;
    }

    if (set->second.get().emplace(identifier).second)
    {
        // Fire signal only when something got added
        set->second.signal_setChanged().emit();
    }
}

void FavouritesManager::removeFavourite(const std::string& typeName, const std::string& identifier)
{
    if (typeName.empty() || identifier.empty()) return;

    auto set = _favouritesByType.find(typeName);

    if (set == _favouritesByType.end())
    {
        return;
    }

    if (set->second.get().erase(identifier) > 0)
    {
        // Fire signal only when something got removed
        set->second.signal_setChanged().emit();
    }
}

bool FavouritesManager::isFavourite(const std::string& typeName, const std::string& identifier)
{
    if (typeName.empty() || identifier.empty()) return false;

    auto set = _favouritesByType.find(typeName);

    return set != _favouritesByType.end() ? set->second.get().count(identifier) > 0 : false;
}

std::set<std::string> FavouritesManager::getFavourites(const std::string& typeName)
{
    if (typeName.empty())
    {
        return std::set<std::string>();
    }

    auto set = _favouritesByType.find(typeName);

    return set != _favouritesByType.end() ? set->second.get() : std::set<std::string>();
}

sigc::signal<void>& FavouritesManager::getSignalForType(const std::string& typeName)
{
    if (typeName.empty())
    {
        throw std::invalid_argument("No signal for empty typenames");
    }

    auto set = _favouritesByType.find(typeName);

    if (set == _favouritesByType.end())
    {
        set = _favouritesByType.emplace(typeName, FavouriteSet(typeName)).first;
    }

    return set->second.signal_setChanged();
}

const std::string& FavouritesManager::getName() const
{
    static std::string _name(MODULE_FAVOURITES_MANAGER);
    return _name;
}

const StringSet& FavouritesManager::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.emplace(MODULE_XMLREGISTRY);
    }

    return _dependencies;
}

void FavouritesManager::initialiseModule(const IApplicationContext&)
{
    // Up to version 2.10.0, the MediaBrowser favourites were stored in this path
    importLegacySet(RKEY_MEDIABROWSER_LEGACY_ROOT, decl::getTypeName(decl::Type::Material));

    // Load from the legacy paths (pre-3.1.0)
    std::string root = RKEY_FAVOURITES_ROOT;

    importLegacySet(root + RKEY_LEGACY_SUBPATH_MATERIALS, decl::getTypeName(decl::Type::Material));
    importLegacySet(root + RKEY_LEGACY_SUBPATH_ENTITYDEFS, decl::getTypeName(decl::Type::EntityDef));
    importLegacySet(root + RKEY_LEGACY_SUBPATH_SOUNDSHADERS, decl::getTypeName(decl::Type::SoundShader));
    importLegacySet(root + RKEY_LEGACY_SUBPATH_PARTICLES, decl::getTypeName(decl::Type::Particle));
    importLegacySet(root + RKEY_LEGACY_SUBPATH_MODELS, "model");

    // Load the rest of the types from the remaining regular paths
    auto nodes = GlobalRegistry().findXPath(root + "/*");

    for (const auto& node : nodes)
    {
        auto typeName = node.getName();

        if (typeName.empty()) continue;

        // Ensure a set with that typename exists
        auto set = _favouritesByType.find(typeName);

        if (set == _favouritesByType.end())
        {
            set = _favouritesByType.emplace(typeName, FavouriteSet(typeName)).first;
        }

        // Append all favourites in that node to the set
        set->second.loadFromRegistry(root);
    }
}

void FavouritesManager::importLegacySet(const std::string& path, const std::string& targetTypeName)
{
    auto oldSet = FavouriteSet(); // untyped set to be able to use the raw path
    oldSet.loadFromRegistry(path);

    for (const auto& identifier : oldSet.get())
    {
        addFavourite(targetTypeName, identifier);
    }

    GlobalRegistry().deleteXPath(path);
}

void FavouritesManager::shutdownModule()
{
    std::string root = RKEY_FAVOURITES_ROOT;
    GlobalRegistry().deleteXPath(RKEY_FAVOURITES_ROOT);

    // Save favourites to registry
    for (const auto& [_, set] :_favouritesByType)
    {
        set.saveToRegistry(root);
    }

    // Clear observers
    for (auto& pair : _favouritesByType)
    {
        pair.second.signal_setChanged().clear();
    }
}

module::StaticModuleRegistration<FavouritesManager> favouritesManagerModule;

}
