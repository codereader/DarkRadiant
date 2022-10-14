#pragma once

#include "imodule.h"

class wxWindow;

namespace ui
{
/**
 * User control interface, to be implemented by all widgets that
 * can be packed into DarkRadiant's main frame or group dialog tabs.
 */
class IUserControl
{
public:
    virtual ~IUserControl() {}

    using Ptr = std::shared_ptr<IUserControl>;

    // Returns the name of this control
    virtual std::string getControlName() = 0;

    // Creates a new wxWidget window for packing into a dialog or sizer
    // Widget ownership is transferred to the caller, IUserControl implementations
    // will not delete the returned window
    virtual wxWindow* createWidget(wxWindow* parent) = 0;
};

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
};

}

constexpr const char* const MODULE_USERINTERFACE = "UserInterfaceModule";

// The accessor function
inline ui::IUserInterfaceModule& GlobalUserInterface()
{
    static module::InstanceReference<ui::IUserInterfaceModule> _reference(MODULE_USERINTERFACE);
    return _reference;
}
