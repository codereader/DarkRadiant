#pragma once

#include <string>

namespace ui
{
/**
 * Interface of an object that supports saving and loading its state
 * to and from a registry key, respectively.
 */
class IPersistableObject
{
public:
    virtual ~IPersistableObject() {}

    // Load the state of this object from the given registry element
    virtual void loadFromPath(const std::string& registryKey) = 0;

    // Save the state of this object to the given registry element
    // It is allowed to create sub-elements.
    virtual void saveToPath(const std::string& registryKey) = 0;
};

}
