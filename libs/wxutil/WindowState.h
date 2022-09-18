#pragma once

#include <string>
#include <vector>

#include "ui/iwindowstate.h"

namespace wxutil
{

/**
 * Helps saving and restoring the state of a window and any registered child objects.
 * The state will be saved to a single registry element, whose path is determined by
 * the name passed to the constructor.
 */
class WindowState
{
private:
    std::string _windowName;
    std::vector< ui::IPersistableObject*> _objects;

public:
    WindowState(const std::string& windowName);

    virtual ~WindowState() {}

    // Add a persistable object to the list of elements to be saved/restored
    virtual void registerObject(ui::IPersistableObject* object);

    // Save the current state to the associated registry key
    virtual void save();

    // Load the state from the associated registry key
    virtual void restore();

private:
    std::string getWindowStatePath() const;
};

}
