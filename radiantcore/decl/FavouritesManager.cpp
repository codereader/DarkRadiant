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
}

void FavouritesManager::addFavourite(decl::Type type, const std::string& path)
{
    auto set = _favouritesByType.find(type);

    if (set == _favouritesByType.end())
    {
        set = _favouritesByType.emplace(type, FavouriteSet()).first;
    }

    set->second.get().emplace(path);
}

void FavouritesManager::removeFavourite(decl::Type type, const std::string& path)
{
    auto set = _favouritesByType.find(type);

    if (set == _favouritesByType.end())
    {
        return;
    }

    set->second.get().erase(path);
}

bool FavouritesManager::isFavourite(decl::Type type, const std::string& path)
{
    auto set = _favouritesByType.find(type);

    return set != _favouritesByType.end() ? set->second.get().count(path) > 0 : false;
}

std::set<std::string> FavouritesManager::getFavourites(decl::Type type)
{
    auto set = _favouritesByType.find(type);

    return set != _favouritesByType.end() ? set->second.get() : std::set<std::string>();
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
}

void FavouritesManager::shutdownModule()
{
    std::string root = RKEY_FAVOURITES_ROOT;
    GlobalRegistry().deleteXPath(RKEY_FAVOURITES_ROOT);

    // Save favourites to registry
    _favouritesByType[Type::Material].saveToRegistry(root + RKEY_SUBPATH_MATERIALS);
}

module::StaticModule<FavouritesManager> favouritesManagerModule;

}
