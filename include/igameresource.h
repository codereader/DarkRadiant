#pragma once

#include <string>

namespace game
{

/**
 * Interface for objects representing resources which were defined within
 * either a directory or a PK4 archive belonging to a particular mod/game. Provides
 * a single method to retrieve the name of the mod that contains this particular
 * resource.
 */
class IResource
{
public:
    virtual ~IResource() {}

    /**
     * Return the name of the mod which owns this resource object. Objects that
     * are not part of a separate mod are found in the base/ directory, and will
     * return "base" from this function.
     */
    virtual std::string getModName() const = 0;
};

}
