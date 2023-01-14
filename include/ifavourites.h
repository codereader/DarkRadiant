#pragma once

#include <set>
#include <sigc++/signal.h>
#include "imodule.h"

namespace game
{

/**
 * Favourite Management interface keeping track of resources
 * that have been tagged as favourite by the user.
 *
 * Each favourite obejct has a typename and a (unique) identifier.
 * The typename is treated case-insensitively - it is used as tag identifier
 * when persisting the set, therefore it must consist of word characters only.
 */
class IFavouritesManager :
    public RegisterableModule
{
public:
    virtual ~IFavouritesManager() {}

    /**
     * Adds the given favourite object to the set of favourites
     *
     * @typeName: Type name of the favourite, like "prefab" or "material"
     * Favourites with the same type name are grouped.
     *
     * @identifier: The identifier of this favourite object,
     * like a file path or a declaration name
     */
    virtual void addFavourite(const std::string& typeName, const std::string& identifier) = 0;

    // Removes the favourite with the given type and identifier from the set of favourites
    virtual void removeFavourite(const std::string& typeName, const std::string& identifier) = 0;

    // Returns true if the given type/name combination is listed as favourite
    virtual bool isFavourite(const std::string& typeName, const std::string& identifier) = 0;

    // Returns the whole set of favourites for the given type name
    virtual std::set<std::string> getFavourites(const std::string& typeName) = 0;

    // Returns the changed signal for the given type - will be fired when the set changes
    // Requesting a signal for an empty typename will trigger a std::invalid_argument exception
    virtual sigc::signal<void>& getSignalForType(const std::string& typeName) = 0;
};

}

constexpr const char* const MODULE_FAVOURITES_MANAGER("FavouritesManager");

inline game::IFavouritesManager& GlobalFavouritesManager()
{
    static module::InstanceReference<game::IFavouritesManager> _reference(MODULE_FAVOURITES_MANAGER);
    return _reference;
}

