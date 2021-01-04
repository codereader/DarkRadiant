#pragma once

#include <set>
#include <sigc++/signal.h>
#include "imodule.h"
#include "idecltypes.h"

namespace decl
{

class IFavouritesManager :
    public RegisterableModule
{
public:
    virtual ~IFavouritesManager() {}

    // Adds the given declaration (as identified by the given path) to 
    // the set of favourites
    virtual void addFavourite(decl::Type type, const std::string& path) = 0;

    // Removes the given declaration from the favourites set
    virtual void removeFavourite(decl::Type type, const std::string& path) = 0;

    // Returns true if the given declaration is listed as favourite
    virtual bool isFavourite(decl::Type type, const std::string& path) = 0;

    // Returns the whole set of favourites for the given declaration type
    virtual std::set<std::string> getFavourites(decl::Type type) = 0;

    // Returns the changed signal for the given type - will be fired when the set changes
    virtual sigc::signal<void>& getSignalForType(decl::Type type) = 0;
};

}

const char* const MODULE_FAVOURITES_MANAGER("FavouritesManager");

inline decl::IFavouritesManager& GlobalFavouritesManager()
{
    static module::InstanceReference<decl::IFavouritesManager> _reference(MODULE_FAVOURITES_MANAGER);
    return _reference;
}

