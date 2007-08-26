#ifndef MODRESOURCE_H_
#define MODRESOURCE_H_

#include <string>

/**
 * Interface for objects representing resources which were defined within
 * either a directory or a PK4 archive belonging to a particular mod. Provides
 * a single method to retrieve the name of the mod that contains this particular
 * resource.
 */
class ModResource
{
public:
    
    /**
     * Return the name of the mod which owns this resource object. Objects that
     * are not part of a separate mod are found in the base/ directory, and will
     * return "base" from this function.
     */
    virtual std::string getModName() const = 0;
};

#endif /*MODRESOURCE_H_*/
