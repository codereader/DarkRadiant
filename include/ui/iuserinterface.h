#pragma once

#include "imodule.h"
#include "iusercontrol.h"

namespace ui
{

/**
 * @brief Module responsible for creating all dockable UI panels.
 *
 * This is a dynamically populated factory object which maintains a map of
 * IUserControlCreator objects indexed by name, each of which can create a dock widget of a
 * specific type. Any part of the code (including plugins) can register new control
 * creators.
 */
class IUserInterfaceModule: public RegisterableModule
{
public:
    ~IUserInterfaceModule() override {}

    // Runs the specified action in the UI thread
    // this happens when the application has a chance to, usually during event processing
    // This method is safe to be called from any thread.
    virtual void dispatch(const std::function<void()>& action) = 0;

    /**
     * @brief Register a new control
     *
     * After registration, clients can acquire the control by invoking the findControl()
     * method.
     */
    virtual void registerControl(IUserControlCreator::Ptr control) = 0;

    /**
     * @brief Lookup a control creator by name
     *
     * @param name
     * The name of the control creator, which was exposed via the
     * IUserControlCreator::getControlName() method.
     *
     * @return IUserControlCreator::Ptr
     * Pointer to the matching control creator, or null if no control creator with this name
     * could be found.
     */
    virtual IUserControlCreator::Ptr findControl(const std::string& name) = 0;

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
