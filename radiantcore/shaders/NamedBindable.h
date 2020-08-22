#pragma once

#include <iimage.h>
#include <string>
#include <memory>

/**
 * \brief
 * Interface for a BindableTexture which also has a string identifier for
 * caching.
 */
class NamedBindable
: public BindableTexture
{
public:

    /**
     * \brief
     * Get the string identifier.
     */
    virtual std::string getIdentifier() const = 0;
};
typedef std::shared_ptr<NamedBindable> NamedBindablePtr;


