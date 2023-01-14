#include "SceneManipulateMouseTool.h"

#include "i18n.h"
#include "iselection.h"
#include "selection/SelectionVolume.h"
#include "Rectangle.h"

namespace ui
{

const std::string& SceneManipulateMouseTool::getName()
{
    static std::string name("ManipulateMouseTool");
    return name;
}

const std::string& SceneManipulateMouseTool::getDisplayName()
{
    static std::string displayName(_("Manipulate"));
    return displayName;
}

selection::IManipulator::Ptr SceneManipulateMouseTool::getActiveManipulator()
{
    return GlobalSelectionSystem().getActiveManipulator();
}

bool SceneManipulateMouseTool::manipulationIsPossible()
{
    auto activeManipulator = getActiveManipulator();
    assert(activeManipulator);

    bool dragComponentMode = activeManipulator->getType() == selection::IManipulator::Drag &&
        GlobalSelectionSystem().getSelectionMode() == selection::SelectionMode::Component;

    return dragComponentMode || !nothingSelected();
}

Matrix4 SceneManipulateMouseTool::getPivot2World()
{
    return GlobalSelectionSystem().getPivot2World();
}

void SceneManipulateMouseTool::onManipulationStart()
{
    GlobalSelectionSystem().onManipulationStart();
}

void SceneManipulateMouseTool::onManipulationChanged()
{
    GlobalSelectionSystem().onManipulationChanged();
}

void SceneManipulateMouseTool::onManipulationCancelled()
{
    GlobalSelectionSystem().onManipulationCancelled();
}

void SceneManipulateMouseTool::onManipulationFinished()
{
    GlobalSelectionSystem().onManipulationEnd();
}

bool SceneManipulateMouseTool::gridIsEnabled()
{
    return true;
}

bool SceneManipulateMouseTool::nothingSelected() const
{
    switch (GlobalSelectionSystem().getSelectionMode())
    {
    case selection::SelectionMode::Component:
        return GlobalSelectionSystem().countSelectedComponents() == 0;

    case selection::SelectionMode::GroupPart:
    case selection::SelectionMode::Primitive:
    case selection::SelectionMode::Entity:
        return GlobalSelectionSystem().countSelected() == 0;

    default:
        return false;
    };
}

}
