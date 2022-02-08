#include "FavouritesManager.h"

#include "iregistry.h"
#include "module/StaticModule.h"

namespace decl
{

namespace
{
    const char* const RKEY_MEDIABROWSER_LEGACY_ROOT = "user/ui/mediaBrowser/favourites";
    const char* const RKEY_FAVOURITES_ROOT = "user/ui/favourites";
    const char* const RKEY_SUBPATH_MATERIALS = "/materials";
    const char* const RKEY_SUBPATH_ENTITYDEFS = "/entityDefs";
    const char* const RKEY_SUBPATH_SOUNDSHADERS = "/soundShaders";
    const char* const RKEY_SUBPATH_MODELS = "/models";
    const char* const RKEY_SUBPATH_PARTICLES = "/particles";
}

void FavouritesManager::addFavourite(decl::Type type, const std::string& path)
{
    if (path.empty() || type == decl::Type::None) return;

    auto set = _favouritesByType.find(type);

    if (set == _favouritesByType.end())
    {
        set = _favouritesByType.emplace(type, FavouriteSet()).first;
    }

    if (set->second.get().emplace(path).second)
    {
        // Fire signal only when something got added
        set->second.signal_setChanged().emit();
    }
}

void FavouritesManager::removeFavourite(decl::Type type, const std::string& path)
{
    if (path.empty() || type == decl::Type::None) return;

    auto set = _favouritesByType.find(type);

    if (set == _favouritesByType.end())
    {
        return;
    }

    if (set->second.get().erase(path) > 0)
    {
        // Fire signal only when something got removed
        set->second.signal_setChanged().emit();
    }
}

bool FavouritesManager::isFavourite(decl::Type type, const std::string& path)
{
    if (path.empty() || type == decl::Type::None) return false;

    auto set = _favouritesByType.find(type);

    return set != _favouritesByType.end() ? set->second.get().count(path) > 0 : false;
}

std::set<std::string> FavouritesManager::getFavourites(decl::Type type)
{
    if (type == decl::Type::None)
    {
        return std::set<std::string>();
    }

    auto set = _favouritesByType.find(type);

    return set != _favouritesByType.end() ? set->second.get() : std::set<std::string>();
}

sigc::signal<void>& FavouritesManager::getSignalForType(decl::Type type)
{
    if (type == decl::Type::None)
    {
        throw std::logic_error("No signal for decl::Type::None");
    }

    auto set = _favouritesByType.find(type);

    if (set == _favouritesByType.end())
    {
        set = _favouritesByType.emplace(type, FavouriteSet()).first;
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
    _favouritesByType[Type::Material].loadFromRegistry(RKEY_MEDIABROWSER_LEGACY_ROOT);

    // Get rid of this old key after importing its data
    GlobalRegistry().deleteXPath(RKEY_MEDIABROWSER_LEGACY_ROOT);

    // Load from the regular paths
    std::string root = RKEY_FAVOURITES_ROOT;
    _favouritesByType[Type::Material].loadFromRegistry(root + RKEY_SUBPATH_MATERIALS);
    _favouritesByType[Type::EntityDef].loadFromRegistry(root + RKEY_SUBPATH_ENTITYDEFS);
    _favouritesByType[Type::SoundShader].loadFromRegistry(root + RKEY_SUBPATH_SOUNDSHADERS);
    _favouritesByType[Type::Model].loadFromRegistry(root + RKEY_SUBPATH_MODELS);
    _favouritesByType[Type::Particle].loadFromRegistry(root + RKEY_SUBPATH_PARTICLES);
}

void FavouritesManager::shutdownModule()
{
    std::string root = RKEY_FAVOURITES_ROOT;
    GlobalRegistry().deleteXPath(RKEY_FAVOURITES_ROOT);

    // Save favourites to registry
    _favouritesByType[Type::Material].saveToRegistry(root + RKEY_SUBPATH_MATERIALS);
    _favouritesByType[Type::EntityDef].saveToRegistry(root + RKEY_SUBPATH_ENTITYDEFS);
    _favouritesByType[Type::SoundShader].saveToRegistry(root + RKEY_SUBPATH_SOUNDSHADERS);
    _favouritesByType[Type::Model].saveToRegistry(root + RKEY_SUBPATH_MODELS);
    _favouritesByType[Type::Particle].saveToRegistry(root + RKEY_SUBPATH_PARTICLES);

    // Clear observers
    for (auto& pair : _favouritesByType)
    {
        pair.second.signal_setChanged().clear();
    }
}

module::StaticModuleRegistration<FavouritesManager> favouritesManagerModule;

}
