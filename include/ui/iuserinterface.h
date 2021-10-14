#pragma once

#include "imodule.h"

namespace ui
{

class IUserInterfaceModule :
    public RegisterableModule
{
public:
    virtual ~IUserInterfaceModule() {}

    // Runs the specified action in the UI thread 
    // this happens when the application has a chance to, usually during event processing
    // This method is safe to be called from any thread.
    virtual void dispatch(const std::function<void()>& action) = 0;
};

}

constexpr const char* const MODULE_USERINTERFACE = "UserInterfaceModule";

// The accessor function
inline ui::IUserInterfaceModule& GlobalUserInterface()
{
    static module::InstanceReference<ui::IUserInterfaceModule> _reference(MODULE_USERINTERFACE);
    return _reference;
}
