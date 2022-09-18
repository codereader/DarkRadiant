#include "WindowState.h"

namespace wxutil
{

namespace
{
    constexpr const char* const RKEY_WINDOW_STATES = "user/ui/windowStates/";
}

WindowState::WindowState(const std::string& windowName) :
    _windowName(windowName)
{}

void WindowState::registerObject(ui::IPersistableObject* object)
{
    _objects.push_back(object);
}

void WindowState::save()
{
    auto windowStatePath = getWindowStatePath();

    // Compatibility: do nothing if a window didn't specify a name
    if (windowStatePath.empty()) return;

    for (auto object : _objects)
    {
        object->saveToPath(windowStatePath);
    }
}

void WindowState::restore()
{
    auto windowStatePath = getWindowStatePath();

    // Compatibility: do nothing if a window didn't specify a name
    if (windowStatePath.empty()) return;

    for (auto object : _objects)
    {
        object->loadFromPath(windowStatePath);
    }
}

std::string WindowState::getWindowStatePath() const
{
    if (_windowName.empty()) return {};

    return RKEY_WINDOW_STATES + _windowName;
}

}
