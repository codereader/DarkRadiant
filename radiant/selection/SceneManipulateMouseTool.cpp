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

bool SceneManipulateMouseTool::selectManipulator(const render::View& view, const Vector2& devicePoint, const Vector2& deviceEpsilon)
{
    auto activeManipulator = getActiveManipulator();
    assert(activeManipulator);

    bool dragComponentMode = activeManipulator->getType() == selection::IManipulator::Drag &&
        GlobalSelectionSystem().Mode() == SelectionSystem::eComponent;

    if (!nothingSelected() || dragComponentMode)
    {
        // Unselect any currently selected manipulators to be sure
        activeManipulator->setSelected(false);

        const Matrix4& pivot2World = GlobalSelectionSystem().getPivot2World();

        // Perform a selection test on this manipulator's components
        render::View scissored(view);
        ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(devicePoint, deviceEpsilon));

        SelectionVolume test(scissored);
        activeManipulator->testSelect(test, pivot2World);

        // Save the pivot2world matrix
        _pivot2worldStart = pivot2World;

        GlobalSelectionSystem().onManipulationStart();

        // This is true, if a manipulator could be selected
        _manipulationActive = activeManipulator->isSelected();

        // is a manipulator selected / the pivot moving?
        if (_manipulationActive)
        {
            activeManipulator->getActiveComponent()->beginTransformation(_pivot2worldStart, view, devicePoint);

            _deviceStart = devicePoint;

            _undoBegun = false;
        }
    }

    return _manipulationActive;
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

bool SceneManipulateMouseTool::nothingSelected() const
{
    switch (GlobalSelectionSystem().Mode())
    {
    case SelectionSystem::eComponent:
        return GlobalSelectionSystem().countSelectedComponents() == 0;

    case SelectionSystem::eGroupPart:
    case SelectionSystem::ePrimitive:
    case SelectionSystem::eEntity:
        return GlobalSelectionSystem().countSelected() == 0;

    default:
        return false;
    };
}

}
