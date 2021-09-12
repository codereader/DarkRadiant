#include "TextureToolManipulateMouseTool.h"

#include <wx/utils.h>
#include "i18n.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "registry/registry.h"
#include "selection/Device.h"
#include "Rectangle.h"
#include "selection/Pivot2World.h"
#include "selection/SelectionVolume.h"
#include <fmt/format.h>
#include "string/split.h"
#include "textool/TexTool.h"

namespace ui
{

namespace
{
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

TextureToolManipulateMouseTool::TextureToolManipulateMouseTool() :
    _selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON)),
	_manipulationActive(false),
	_undoBegun(false)
{}

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

MouseTool::Result TextureToolManipulateMouseTool::onMouseDown(Event& ev)
{
    _view = render::View(ev.getInteractiveView().getVolumeTest());

    Vector2 epsilon(_selectEpsilon / ev.getInteractiveView().getDeviceWidth(),
                    _selectEpsilon / ev.getInteractiveView().getDeviceHeight());

    if (selectManipulator(_view, ev.getDevicePosition(), epsilon))
    {
        return Result::Activated;
    }

    return Result::Ignored; // not handled
}

MouseTool::Result TextureToolManipulateMouseTool::onMouseMove(Event& ev)
{
    // Get the view afresh each time, chasemouse might have changed the view since onMouseDown
    _view = render::View(ev.getInteractiveView().getVolumeTest());

    handleMouseMove(_view, ev.getDevicePosition());

    return Result::Continued;
}

MouseTool::Result TextureToolManipulateMouseTool::onMouseUp(Event& ev)
{
    // Notify the selectionsystem about the finished operation
    endMove();
    return Result::Finished;
}

void TextureToolManipulateMouseTool::onMouseCaptureLost(IInteractiveView& view)
{
    cancelMove();
}

TextureToolManipulateMouseTool::Result TextureToolManipulateMouseTool::onCancel(IInteractiveView&)
{
    cancelMove();

    return Result::Finished;
}

unsigned int TextureToolManipulateMouseTool::getPointerMode()
{
    return PointerMode::Capture;
}

unsigned int TextureToolManipulateMouseTool::getRefreshMode()
{
    return RefreshMode::Force | RefreshMode::AllViews; // update cam view too
}

bool TextureToolManipulateMouseTool::selectManipulator(const render::View& view, const Vector2& devicePoint, const Vector2& deviceEpsilon)
{
	const auto& activeManipulator = TexTool::Instance().getActiveManipulator();

	assert(activeManipulator);
	
#if 0
	bool dragComponentMode = activeManipulator->getType() == selection::Manipulator::Drag && 
		GlobalSelectionSystem().Mode() == SelectionSystem::eComponent;
#endif
	if (true/*!nothingSelected()/* || dragComponentMode*/)
	{
		// Unselect any currently selected manipulators to be sure
		activeManipulator->setSelected(false);

		const Matrix4& pivot2World = TexTool::Instance().getPivot2World();

		// Test, if the current manipulator can be selected
		if (true/*!nothingSelected()/* || dragComponentMode*/)
		{
			render::View scissored(view);
			ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(devicePoint, deviceEpsilon));

			SelectionVolume test(scissored);

			// The manipulator class checks on its own, if any of its components can be selected
			activeManipulator->testSelect(test, pivot2World);
		}

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
		}
	}

	return _manipulationActive;
}

void TextureToolManipulateMouseTool::handleMouseMove(const render::View& view, const Vector2& devicePoint)
{
	const auto& activeManipulator = TexTool::Instance().getActiveManipulator();
	assert(activeManipulator);

	// Check if the active manipulator is selected in the first place
	if (!activeManipulator->isSelected()) return;

	// Initalise the undo system, if not yet done
	if (!_undoBegun)
	{
		_undoBegun = true;
		GlobalUndoSystem().start();
	}

#ifdef _DEBUG
	Matrix4 device2pivot = constructDevice2Pivot(_pivot2worldStart, view);
	Matrix4 pivot2device = constructPivot2Device(_pivot2worldStart, view);

	Vector4 pivotDev = pivot2device.transform(Vector4(0,0,0,1));

	_debugText = fmt::format("\nPivotDevice x,y,z,w = ({0:5.3f} {1:5.3f} {2:5.3f} {3:5.3f})", pivotDev.x(), pivotDev.y(), pivotDev.z(), pivotDev.w());

	_debugText += fmt::format("\nStart x,y = ({0:5.3f} {1:5.3f})", _deviceStart.x(), _deviceStart.y());
	_debugText += fmt::format("\nCurrent x,y = ({0:5.3f} {1:5.3f})", devicePoint.x(), devicePoint.y());

	double pivotDistanceDeviceSpace = pivot2device.tz();

	Vector3 worldPosH = device2pivot.transform(Vector4(devicePoint.x(), devicePoint.y(), pivotDistanceDeviceSpace, 1)).getProjected();

	_debugText += fmt::format("\nDev2Pivot x,y,z = ({0:5.3f} {1:5.3f} {2:5.3f})", worldPosH.x(), worldPosH.y(), worldPosH.z());

	worldPosH = device2pivot.transform(pivotDev).getProjected();
	_debugText += fmt::format("\nTest reversal x,y,z = ({0:5.3f} {1:5.3f} {2:5.3f})", worldPosH.x(), worldPosH.y(), worldPosH.z());
#endif

	// Query keyboard modifier state and pass them as flags
	int constraintFlag = selection::IManipulator::Component::Constraint::Unconstrained;
	constraintFlag |= wxGetKeyState(WXK_SHIFT) ? selection::IManipulator::Component::Constraint::Type1 : 0;
	constraintFlag |= wxGetKeyState(WXK_ALT) ? selection::IManipulator::Component::Constraint::Type3 : 0;

	// Grid constraint is ON by default, unless CTRL is held
	constraintFlag |= wxGetKeyState(WXK_CONTROL) ? 0 : selection::IManipulator::Component::Constraint::Grid;

	// Get the component of the currently active manipulator (done by selection test)
	// and call the transform method
	activeManipulator->getActiveComponent()->transform(_pivot2worldStart, view, devicePoint, constraintFlag);

	TexTool::Instance().onManipulationChanged();
}

void TextureToolManipulateMouseTool::freezeTransforms()
{
    TexTool::Instance().onManipulationEnd();
}

void TextureToolManipulateMouseTool::endMove()
{
	freezeTransforms();

	const auto& activeManipulator = TexTool::Instance().getActiveManipulator();
	assert(activeManipulator);

	_manipulationActive = false;

	// Update the views
	SceneChangeNotify();

	// If we started an undoable operation, end it now and tell the console what happened
	if (_undoBegun)
	{
		_undoBegun = false;

		// Finish the undo move
		GlobalUndoSystem().finish("manipulateTexture");
	}
}

void TextureToolManipulateMouseTool::cancelMove()
{
	const auto& activeManipulator = TexTool::Instance().getActiveManipulator();
	assert(activeManipulator);

	_manipulationActive = false;

    TexTool::Instance().onManipulationCancelled();

	if (_undoBegun)
	{
		// Cancel the undo operation, if one has been begun
		GlobalUndoSystem().cancel();
		_undoBegun = false;
	}

	// Update the views
	SceneChangeNotify();
}

bool TextureToolManipulateMouseTool::nothingSelected() const
{
    return TexTool::Instance().countSelected() == 0;
}

void TextureToolManipulateMouseTool::renderOverlay()
{
#ifdef _DEBUG
	std::vector<std::string> lines;
	string::split(lines, _debugText, "\n");

	for (std::size_t i = 0; i < lines.size(); ++i)
	{
		glRasterPos3f(1.0f, 15.0f + (12.0f*lines.size() - 1) - 12.0f*i, 0.0f);
		GlobalOpenGL().drawString(lines[i]);
	}
#endif
}

}
