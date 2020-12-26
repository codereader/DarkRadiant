#pragma once

#include "ifavourites.h"

namespace decl
{

class FavouritesManager :
    public IFavouritesManager
{
private:
    std::map<Type, std::set<std::string>> _favouritesByType;

public:
    void addFavourite(decl::Type type, const std::string& path) override;
    void removeFavourite(decl::Type type, const std::string& path) override;
    bool isFavourite(decl::Type type, const std::string& path) override;
    std::set<std::string> getFavourites(decl::Type type) override;

    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext&) override;
    void shutdownModule() override;
};

}
