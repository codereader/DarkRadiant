#include "FavouritesManager.h"

#include "iregistry.h"
#include "module/StaticModule.h"

namespace decl
{

void FavouritesManager::addFavourite(decl::Type type, const std::string& path)
{
    auto set = _favouritesByType.find(type);

    if (set == _favouritesByType.end())
    {
        set = _favouritesByType.emplace(type, std::set<std::string>()).first;
    }

    set->second.emplace(path);
}

void FavouritesManager::removeFavourite(decl::Type type, const std::string& path)
{
    auto set = _favouritesByType.find(type);

    if (set == _favouritesByType.end())
    {
        return;
    }

    set->second.erase(path);
}

bool FavouritesManager::isFavourite(decl::Type type, const std::string& path)
{
    auto set = _favouritesByType.find(type);

    return set != _favouritesByType.end() ? set->second.count(path) > 0 : false;
}

std::set<std::string> FavouritesManager::getFavourites(decl::Type type)
{
    auto set = _favouritesByType.find(type);

    return set != _favouritesByType.end() ? set->second : std::set<std::string>();
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
    // TODO: Load favourites from registry
}

void FavouritesManager::shutdownModule()
{
    // TODO: Save favourites to registry
}

module::StaticModule<FavouritesManager> favouritesManagerModule;

}
