#pragma once

#include <memory>
#include <string>

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

    // Returns the name of this control. This is an identifier corresponding to the
    // UserControl enumeration, like UserControl::Camera, or a plugin-defined identifier
    virtual std::string getControlName() = 0;

    // A visible, localised identifier used for tab captions and window titles
    virtual std::string getDisplayName() = 0;

    // Optional icon file name to use for tab captions, e.g. "icon_texture.png"
    virtual std::string getIcon() { return {}; }

    // Creates a new wxWidget window for packing into a dialog or sizer
    // Widget ownership is transferred to the caller, IUserControl implementations
    // will not delete the returned window
    virtual wxWindow* createWidget(wxWindow* parent) = 0;
};

// Predefined known user control names
struct UserControl
{
    constexpr static const char* Camera = "Camera";
    constexpr static const char* Console = "Console";
    constexpr static const char* EntityInspector = "EntityInspector";
    constexpr static const char* MediaBrowser = "MediaBrowser";
    constexpr static const char* OrthoView = "OrthoView";
    constexpr static const char* TextureBrowser = "TextureBrowser";
};

}
