#pragma once

#include "imodule.h"
#include "iusercontrol.h"

namespace ui
{

class IUserInterfaceModule :
    public RegisterableModule
{
public:
    ~IUserInterfaceModule() override {}

    // Runs the specified action in the UI thread 
    // this happens when the application has a chance to, usually during event processing
    // This method is safe to be called from any thread.
    virtual void dispatch(const std::function<void()>& action) = 0;

    /**
     * Registers a new control. After registration, clients can acquire
     * the control by invoking the findControl() method.
     */
    virtual void registerControl(const IUserControl::Ptr& control) = 0;

    /**
     * Looks up the user control (interface) by name.
     * Returns an empty reference if nothing matches.
     */
    virtual IUserControl::Ptr findControl(const std::string& name) = 0;

    /**
     * Unregisters the named control. Does nothing if the name is not registered.
     */
    virtual void unregisterControl(const std::string& controlName) = 0;

    /**
     * Iterate over all registered control names
     */
    virtual void foreachControl(const std::function<void(const std::string&)>& functor) = 0;
};

}

constexpr const char* const MODULE_USERINTERFACE = "UserInterfaceModule";

// The accessor function
inline ui::IUserInterfaceModule& GlobalUserInterface()
{
    static module::InstanceReference<ui::IUserInterfaceModule> _reference(MODULE_USERINTERFACE);
    return _reference;
}
