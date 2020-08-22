#pragma once

#include <string>
#include "imodule.h"

namespace radiant
{

/**
 * Interface to DarkRadiant's clipboard which is able to 
 * store and retrieve a string from and to the system clipboard.
 * Access it through the GlobalClipboard() function.
 *
 * This module might not be present in all configurations
 * so its advisable to check for its presence first.
 */
class IClipboard :
    public RegisterableModule
{
public:
    virtual ~IClipboard() {}

    /// Return the contents of the clipboard as a string
    virtual std::string getString() = 0;

    /// Copy the given string to the system clipboard
    virtual void setString(const std::string& str) = 0;
};

}

const char* const MODULE_CLIPBOARD("Clipboard");

inline radiant::IClipboard& GlobalClipboard()
{
    // Cache the reference locally
    static radiant::IClipboard& _instance(
        *std::static_pointer_cast<radiant::IClipboard>(
            module::GlobalModuleRegistry().getModule(MODULE_CLIPBOARD)
        )
    );
    return _instance;
}
