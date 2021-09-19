#pragma once

#include "itexturetoolmodel.h"
#include "ieventmanager.h"

#include <sigc++/connection.h>

namespace ui
{

// Adaptor class connecting the EventSystem toggles connecting the various 
// modes to the actual state of the backend TextureToolSelectionSystem
class TexToolModeToggles
{
private:
    sigc::connection _activeSelectionModeChanged;
    sigc::connection _activeManipulatorChanged;

public:
    TexToolModeToggles()
    {
        _activeManipulatorChanged = GlobalTextureToolSelectionSystem().signal_activeManipulatorChanged()
            .connect(sigc::mem_fun(this, &TexToolModeToggles::onActiveManipulatorChanged));

        _activeSelectionModeChanged = GlobalTextureToolSelectionSystem().signal_selectionModeChanged()
            .connect(sigc::mem_fun(this, &TexToolModeToggles::onSelectionModeChanged));

        // Manipulator mode toggles
        GlobalEventManager().addToggle("TextureToolRotateMode", [this](bool)
        {
            GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Rotate" });
        });

        GlobalEventManager().addToggle("TextureToolDragMode", [this](bool)
        {
            GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Drag" });
        });

        // Selection mode toggles
        GlobalEventManager().addToggle("TextureToolSurfaceSelectionMode", [this](bool)
        {
            GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Surface" });
        });

        GlobalEventManager().addToggle("TextureToolVertexSelectionMode", [this](bool)
        {
            GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Vertex" });
        });

        onSelectionModeChanged(GlobalTextureToolSelectionSystem().getMode());
        onActiveManipulatorChanged(GlobalTextureToolSelectionSystem().getActiveManipulatorType());
    }

    ~TexToolModeToggles()
    {
        _activeManipulatorChanged.disconnect();
        _activeSelectionModeChanged.disconnect();
    }

private:
    void onActiveManipulatorChanged(selection::IManipulator::Type type)
    {
        GlobalEventManager().setToggled("TextureToolRotateMode", type == selection::IManipulator::Rotate);
        GlobalEventManager().setToggled("TextureToolDragMode", type == selection::IManipulator::Drag);
    }
    
    void onSelectionModeChanged(textool::SelectionMode mode)
    {
        GlobalEventManager().setToggled("TextureToolSurfaceSelectionMode", mode == textool::SelectionMode::Surface);
        GlobalEventManager().setToggled("TextureToolVertexSelectionMode", mode == textool::SelectionMode::Vertex);
    }
};

}
