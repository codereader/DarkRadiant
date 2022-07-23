#pragma once

#include <map>
#include "ifavourites.h"
#include "FavouriteSet.h"
#include "string/string.h"

namespace game
{

class FavouritesManager :
    public IFavouritesManager
{
private:
    std::map<std::string, FavouriteSet, string::ILess> _favouritesByType;

public:
    void addFavourite(const std::string& typeName, const std::string& identifier) override;
    void removeFavourite(const std::string& typeName, const std::string& identifier) override;
    bool isFavourite(const std::string& typeName, const std::string& identifier) override;
    std::set<std::string> getFavourites(const std::string& typeName) override;
    sigc::signal<void>& getSignalForType(const std::string& typeName) override;

    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext&) override;
    void shutdownModule() override;

private:
    void importLegacySet(const std::string& subpath, const std::string& targetTypeName);
};

}
