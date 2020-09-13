#pragma once

#include "iselection.h"
#include <sigc++/connection.h>

namespace ui
{

// Adaptor class connecting the EventSystem toggles of the various 
// selection modes to the actual state of the backend RadiantSelectionSystem.
class SelectionModeToggle
{
private:
	sigc::connection _selectionModeChanged;
	sigc::connection _componentModeChanged;

public:
	SelectionModeToggle()
	{
		_selectionModeChanged = GlobalSelectionSystem().signal_selectionModeChanged()
			.connect(sigc::mem_fun(this, &SelectionModeToggle::onSelectionModeChanged));

		_componentModeChanged = GlobalSelectionSystem().signal_componentModeChanged()
			.connect(sigc::mem_fun(this, &SelectionModeToggle::onComponentModeChanged));

		GlobalEventManager().addToggle("DragVertices", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleComponentSelectionMode", { "Vertex" });
		});

		GlobalEventManager().addToggle("DragEdges", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleComponentSelectionMode", { "Edge" });
		});

		GlobalEventManager().addToggle("DragFaces", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleComponentSelectionMode", { "Face" });
		});
		
		GlobalEventManager().addToggle("DragEntities", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleEntitySelectionMode");
		});

		GlobalEventManager().addToggle("SelectionModeGroupPart", [this](bool)
		{
			GlobalCommandSystem().executeCommand("ToggleGroupPartSelectionMode");
		});

		onSelectionModeChanged(GlobalSelectionSystem().Mode());
		onComponentModeChanged(GlobalSelectionSystem().ComponentMode());
	}

	~SelectionModeToggle()
	{
		_selectionModeChanged.disconnect();
		_componentModeChanged.disconnect();
	}

private:
	void onSelectionModeChanged(SelectionSystem::EMode selectionMode)
	{
		auto componentMode = GlobalSelectionSystem().ComponentMode();

		updateToggleState(selectionMode, componentMode);
	}

	void onComponentModeChanged(SelectionSystem::EComponentMode componentMode)
	{
		auto selectionMode = GlobalSelectionSystem().Mode();

		updateToggleState(selectionMode, componentMode);
	}

	void updateToggleState(SelectionSystem::EMode selectionMode, SelectionSystem::EComponentMode componentMode)
	{
		GlobalEventManager().setToggled("DragVertices",
			selectionMode == SelectionSystem::eComponent && componentMode == SelectionSystem::eVertex);

		GlobalEventManager().setToggled("DragEdges",
			selectionMode == SelectionSystem::eComponent && componentMode == SelectionSystem::eEdge);

		GlobalEventManager().setToggled("DragFaces",
			selectionMode == SelectionSystem::eComponent && componentMode == SelectionSystem::eFace);

		GlobalEventManager().setToggled("DragEntities",
			selectionMode == SelectionSystem::eEntity && componentMode == SelectionSystem::eDefault);

		GlobalEventManager().setToggled("SelectionModeGroupPart",
			selectionMode == SelectionSystem::eGroupPart && componentMode == SelectionSystem::eDefault);
	}
};

}
