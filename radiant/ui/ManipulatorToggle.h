#pragma once

#include "iselection.h"
#include <sigc++/connection.h>

namespace ui
{

// Adaptor class connecting the EventSystem toggles covering the
// various SelectionSystem manipulator mode to the actual state
// of the backend RadiantSelectionSystem.
class ManipulatorToggle
{
private:
	sigc::connection _activeManipulatorChanged;

public:
	ManipulatorToggle()
	{
		_activeManipulatorChanged = GlobalSelectionSystem().signal_activeManipulatorChanged()
			.connect(sigc::mem_fun(this, &ManipulatorToggle::onActiveManipulatorChanged));

		GlobalEventManager().addToggle("ToggleClipper", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleManipulatorMode", { "Clip" });
		});

		GlobalEventManager().addToggle("MouseTranslate", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleManipulatorMode", { "Translate" });
		});

		GlobalEventManager().addToggle("MouseRotate", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleManipulatorMode", { "Rotate" });
		});

		GlobalEventManager().addToggle("MouseDrag", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleManipulatorMode", { "Drag" });
		});

		GlobalEventManager().addToggle("ToggleModelScaleManipulator", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleManipulatorMode", { "ModelScale" });
		});

		onActiveManipulatorChanged(GlobalSelectionSystem().getActiveManipulatorType());
	}

	~ManipulatorToggle()
	{
		_activeManipulatorChanged.disconnect();
	}

private:
	void onActiveManipulatorChanged(selection::Manipulator::Type type)
	{
		GlobalEventManager().setToggled("ToggleClipper", GlobalClipper().clipMode());
		GlobalEventManager().setToggled("MouseTranslate", type == selection::Manipulator::Translate);
		GlobalEventManager().setToggled("MouseRotate", type == selection::Manipulator::Rotate);
		GlobalEventManager().setToggled("MouseDrag", type == selection::Manipulator::Drag);
		GlobalEventManager().setToggled("ToggleModelScaleManipulator", type == selection::Manipulator::ModelScale);
	}
};

}
