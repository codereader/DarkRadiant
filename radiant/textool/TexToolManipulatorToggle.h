#pragma once

#include "itexturetoolmodel.h"
#include "ieventmanager.h"

#include <sigc++/connection.h>

namespace ui
{

// Adaptor class connecting the EventSystem toggles covering the various 
// manipulator mode to the actual state of the backend TextureToolSelectionSystem
class TextureToolManipulatorToggle
{
private:
    sigc::connection _activeManipulatorChanged;

public:
    TextureToolManipulatorToggle()
    {
        _activeManipulatorChanged = GlobalTextureToolSelectionSystem().signal_activeManipulatorChanged()
            .connect(sigc::mem_fun(this, &TextureToolManipulatorToggle::onActiveManipulatorChanged));

        GlobalEventManager().addToggle("TextureToolRotateMode", [this](bool)
        {
            GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Rotate" });
        });

        GlobalEventManager().addToggle("TextureToolDragMode", [this](bool)
        {
            GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Drag" });
        });

        onActiveManipulatorChanged(GlobalTextureToolSelectionSystem().getActiveManipulatorType());
    }

    ~TextureToolManipulatorToggle()
    {
        _activeManipulatorChanged.disconnect();
    }

private:
    void onActiveManipulatorChanged(selection::IManipulator::Type type)
    {
        GlobalEventManager().setToggled("TextureToolRotateMode", type == selection::IManipulator::Rotate);
        GlobalEventManager().setToggled("TextureToolDragMode", type == selection::IManipulator::Drag);
    }
};

}
