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
    // Whitespace and other non-alphanumeric characters are not allowed
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

// The command used to toggle a control: bring it to front if hidden, or hide it if it's a visible floating window
constexpr const char* const TOGGLE_CONTROL_COMMAND = "ToggleControl";

// The command used to make a control the main control (replacing the resident ortho view)
constexpr const char* const TOGGLE_MAIN_CONTROL_COMMAND = "ToggleMainControl";

// The command name used to focus a control, brings it to front, creating it if necessary
constexpr const char* const FOCUS_CONTROL_COMMAND = "FocusControl";

// The command name used to create a new instance of a control
constexpr const char* const CREATE_CONTROL_COMMAND = "CreateControl";

// Prefix used to construct statements like "ToggleLightInspector"
constexpr const char* const TOGGLE_CONTROL_STATEMENT_PREFIX = "Toggle";

// Prefix for statements making a certain control the main control
constexpr const char* const TOGGLE_MAIN_CONTROL_STATEMENT_PREFIX = "ToggleMainControl_";

// Predefined known user control names
struct UserControl
{
    constexpr static const char* Camera = "Camera";
    constexpr static const char* Console = "Console";
    constexpr static const char* FavouritesBrowser = "FavouritesBrowser";
    constexpr static const char* EntityInspector = "EntityInspector";
    constexpr static const char* EntityList = "EntityList";
    constexpr static const char* MediaBrowser = "MediaBrowser";
    constexpr static const char* OrthoView = "OrthoView";
    constexpr static const char* TextureBrowser = "TextureBrowser";
    constexpr static const char* SurfaceInspector = "SurfaceInspector";
    constexpr static const char* PatchInspector = "PatchInspector";
    constexpr static const char* FindAndReplaceMaterial = "FindAndReplaceMaterial";
    constexpr static const char* LightInspector = "LightInspector";
    constexpr static const char* LayerControlPanel = "LayerControlPanel";
    constexpr static const char* TextureTool = "TextureTool";
    constexpr static const char* TransformPanel = "TransformPanel";
    constexpr static const char* MapMergePanel = "MapMergePanel";
    constexpr static const char* AasVisualisationPanel = "AasVisualisationPanel";
    constexpr static const char* OrthoBackgroundPanel = "OrthoBackgroundPanel";
};

}
