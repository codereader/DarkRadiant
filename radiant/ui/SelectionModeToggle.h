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

        GlobalEventManager().addAdvancedToggle("SelectionModeMergeActions", [this](bool)
        {
            GlobalCommandSystem().executeCommand("ToggleMergeActionSelectionMode");

            return GlobalSelectionSystem().getSelectionMode() == selection::SelectionMode::MergeAction;
        });

		onSelectionModeChanged(GlobalSelectionSystem().getSelectionMode());
		onComponentModeChanged(GlobalSelectionSystem().ComponentMode());
	}

	~SelectionModeToggle()
	{
		_selectionModeChanged.disconnect();
		_componentModeChanged.disconnect();
	}

private:
	void onSelectionModeChanged(selection::SelectionMode selectionMode)
	{
		auto componentMode = GlobalSelectionSystem().ComponentMode();

		updateToggleState(selectionMode, componentMode);
	}

	void onComponentModeChanged(selection::ComponentSelectionMode componentMode)
	{
		auto selectionMode = GlobalSelectionSystem().getSelectionMode();

		updateToggleState(selectionMode, componentMode);
	}

	void updateToggleState(selection::SelectionMode selectionMode, selection::ComponentSelectionMode componentMode)
	{
		GlobalEventManager().setToggled("DragVertices",
			selectionMode == selection::SelectionMode::Component && componentMode == selection::ComponentSelectionMode::Vertex);

		GlobalEventManager().setToggled("DragEdges",
			selectionMode == selection::SelectionMode::Component && componentMode == selection::ComponentSelectionMode::Edge);

		GlobalEventManager().setToggled("DragFaces",
			selectionMode == selection::SelectionMode::Component && componentMode == selection::ComponentSelectionMode::Face);

		GlobalEventManager().setToggled("DragEntities",
			selectionMode == selection::SelectionMode::Entity && componentMode == selection::ComponentSelectionMode::Default);

		GlobalEventManager().setToggled("SelectionModeGroupPart",
			selectionMode == selection::SelectionMode::GroupPart && componentMode == selection::ComponentSelectionMode::Default);

        GlobalEventManager().setToggled("SelectionModeMergeActions",
            selectionMode == selection::SelectionMode::MergeAction && componentMode == selection::ComponentSelectionMode::Default);
	}
};

}
