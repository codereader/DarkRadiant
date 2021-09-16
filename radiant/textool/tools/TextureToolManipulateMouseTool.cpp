#include "TextureToolManipulateMouseTool.h"

#include "i18n.h"
#include "selection/Device.h"
#include "Rectangle.h"
#include "selection/Pivot2World.h"
#include "selection/SelectionVolume.h"
#include "textool/TexTool.h"

namespace ui
{

const std::string& TextureToolManipulateMouseTool::getName()
{
    static std::string name("TextureToolManipulateMouseTool");
    return name;
}

const std::string& TextureToolManipulateMouseTool::getDisplayName()
{
    static std::string displayName(_("Manipulate"));
    return displayName;
}

selection::IManipulator::Ptr TextureToolManipulateMouseTool::getActiveManipulator()
{
    return TexTool::Instance().getActiveManipulator();
}

bool TextureToolManipulateMouseTool::selectManipulator(const render::View& view, const Vector2& devicePoint, const Vector2& deviceEpsilon)
{
	auto activeManipulator = getActiveManipulator();
	assert(activeManipulator);

	if (true)
	{
		// Unselect any currently selected manipulators to be sure
		activeManipulator->setSelected(false);

		const Matrix4& pivot2World = TexTool::Instance().getPivot2World();

        // Perform a selection test on this manipulator's components
		render::View scissored(view);
		ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(devicePoint, deviceEpsilon));

		SelectionVolume test(scissored);

		// The manipulator class checks on its own, if any of its components can be selected
		activeManipulator->testSelect(test, pivot2World);

		// Save the pivot2world matrix
		_pivot2worldStart = pivot2World;

        TexTool::Instance().onManipulationStart();

		// This is true, if a manipulator could be selected
		_manipulationActive = activeManipulator->isSelected();

		// is a manipulator selected / the pivot moving?
		if (_manipulationActive)
		{
			activeManipulator->getActiveComponent()->beginTransformation(_pivot2worldStart, view, devicePoint);

			_deviceStart = devicePoint;

			_undoBegun = false;

            TexTool::Instance().forceRedraw();
		}
	}

	return _manipulationActive;
}

void TextureToolManipulateMouseTool::onManipulationChanged()
{
    TexTool::Instance().onManipulationChanged();
}

void TextureToolManipulateMouseTool::freezeTransforms()
{
    TexTool::Instance().onManipulationEnd();
}

void TextureToolManipulateMouseTool::onManipulationCancelled()
{
    TexTool::Instance().onManipulationCancelled();
}

}
