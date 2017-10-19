#include "ManipulateMouseTool.h"

#include <wx/utils.h>
#include "i18n.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "registry/registry.h"
#include "Device.h"
#include "Rectangle.h"
#include "Pivot2World.h"
#include "SelectionTest.h"
#include "SceneWalkers.h"
#include <fmt/format.h>
#include "string/split.h"

namespace ui
{

namespace
{
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

ManipulateMouseTool::ManipulateMouseTool(SelectionSystem& selectionSystem) :
    _selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON)),
    _selectionSystem(selectionSystem),
	_manipulationActive(false),
	_undoBegun(false)
{}

const std::string& ManipulateMouseTool::getName()
{
    static std::string name("ManipulateMouseTool");
    return name;
}

const std::string& ManipulateMouseTool::getDisplayName()
{
    static std::string displayName(_("Manipulate"));
    return displayName;
}

ManipulateMouseTool::Result ManipulateMouseTool::onMouseDown(Event& ev)
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

ManipulateMouseTool::Result ManipulateMouseTool::onMouseMove(Event& ev)
{
    // Get the view afresh each time, chasemouse might have changed the view since onMouseDown
    _view = render::View(ev.getInteractiveView().getVolumeTest());

    handleMouseMove(_view, ev.getDevicePosition());

    return Result::Continued;
}

ManipulateMouseTool::Result ManipulateMouseTool::onMouseUp(Event& ev)
{
    // Notify the selectionsystem about the finished operation
    endMove();
    return Result::Finished;
}

void ManipulateMouseTool::onMouseCaptureLost(IInteractiveView& view)
{
    cancelMove();
}

ManipulateMouseTool::Result ManipulateMouseTool::onCancel(IInteractiveView&)
{
    cancelMove();

    return Result::Finished;
}

unsigned int ManipulateMouseTool::getPointerMode()
{
    return PointerMode::Capture;
}

unsigned int ManipulateMouseTool::getRefreshMode()
{
    return RefreshMode::Force | RefreshMode::AllViews; // update cam view too
}

bool ManipulateMouseTool::selectManipulator(const render::View& view, const Vector2& devicePoint, const Vector2& deviceEpsilon)
{
	const selection::ManipulatorPtr& activeManipulator = _selectionSystem.getActiveManipulator();

	assert(activeManipulator);
	
	bool dragComponentMode = activeManipulator->getType() == selection::Manipulator::Drag && _selectionSystem.Mode() == SelectionSystem::eComponent;

	if (!nothingSelected() || dragComponentMode)
	{
		// Unselect any currently selected manipulators to be sure
		activeManipulator->setSelected(false);

		const Matrix4& pivot2World = _selectionSystem.getPivot2World();

		// Test, if the current manipulator can be selected
		if (!nothingSelected() || dragComponentMode)
		{
			render::View scissored(view);
			ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(devicePoint, deviceEpsilon));

			// The manipulator class checks on its own, if any of its components can be selected
			activeManipulator->testSelect(scissored, pivot2World);
		}

		// Save the pivot2world matrix
		_pivot2worldStart = pivot2World;

		_selectionSystem.onManipulationStart();

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

void ManipulateMouseTool::handleMouseMove(const render::View& view, const Vector2& devicePoint)
{
	const selection::ManipulatorPtr& activeManipulator = _selectionSystem.getActiveManipulator();
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
	int constraintFlag = selection::Manipulator::Component::Constraint::Unconstrained;
	constraintFlag |= wxGetKeyState(WXK_SHIFT) ? selection::Manipulator::Component::Constraint::Type1 : 0;
	constraintFlag |= wxGetKeyState(WXK_ALT) ? selection::Manipulator::Component::Constraint::Type3 : 0;

	// Grid constraint is ON by default, unless CTRL is held
	constraintFlag |= wxGetKeyState(WXK_CONTROL) ? 0 : selection::Manipulator::Component::Constraint::Grid;

	// Get the component of the currently active manipulator (done by selection test)
	// and call the transform method
	activeManipulator->getActiveComponent()->transform(_pivot2worldStart, view, devicePoint, constraintFlag);

	_selectionSystem.onManipulationChanged();
}

void ManipulateMouseTool::freezeTransforms()
{
	GlobalSceneGraph().foreachNode(scene::freezeTransformableNode);

	_selectionSystem.onManipulationEnd();
}

void ManipulateMouseTool::endMove()
{
	freezeTransforms();

	const selection::ManipulatorPtr& activeManipulator = _selectionSystem.getActiveManipulator();
	assert(activeManipulator);

	// greebo: Deselect all faces if we are in brush and drag mode
	if ((_selectionSystem.Mode() == SelectionSystem::ePrimitive || _selectionSystem.Mode() == SelectionSystem::eGroupPart) &&
		activeManipulator->getType() == selection::Manipulator::Drag)
	{
		SelectAllComponentWalker faceSelector(false, SelectionSystem::eFace);
		GlobalSceneGraph().root()->traverse(faceSelector);
	}

	// Remove all degenerated brushes from the scene graph (should emit a warning)
	_selectionSystem.foreachSelected(RemoveDegenerateBrushWalker());

	_manipulationActive = false;
	_selectionSystem.pivotChanged();
	activeManipulator->setSelected(false);

	// Update the views
	SceneChangeNotify();

	// If we started an undoable operation, end it now and tell the console what happened
	if (_undoBegun)
	{
		std::ostringstream command;

		if (activeManipulator->getType() == selection::Manipulator::Translate)
		{
			command << "translateTool";
			//outputTranslation(command);
		}
		else if (activeManipulator->getType() == selection::Manipulator::Rotate)
		{
			command << "rotateTool";
			//outputRotation(command);
		}
		else if (activeManipulator->getType() == selection::Manipulator::Scale)
		{
			command << "scaleTool";
			//outputScale(command);
		}
		else if (activeManipulator->getType() == selection::Manipulator::Drag)
		{
			command << "dragTool";
		}
		else if (activeManipulator->getType() == selection::Manipulator::ModelScale)
		{
			command << "modelScaleTool";
		}

		_undoBegun = false;

		// Finish the undo move
		GlobalUndoSystem().finish(command.str());
	}
}

void ManipulateMouseTool::cancelMove()
{
	const selection::ManipulatorPtr& activeManipulator = _selectionSystem.getActiveManipulator();
	assert(activeManipulator);

	// Unselect any currently selected manipulators to be sure
	activeManipulator->setSelected(false);

	// Tell all the scene objects to revert their transformations
	_selectionSystem.foreachSelected([](const scene::INodePtr& node)
	{
		ITransformablePtr transform = Node_getTransformable(node);

		if (transform)
		{
			transform->revertTransform();
		}

		// In case of entities, we need to inform the child nodes as well
		if (Node_getEntity(node))
		{
			node->foreachNode([&](const scene::INodePtr& child)
			{
				ITransformablePtr transform = Node_getTransformable(child);

				if (transform)
				{
					transform->revertTransform();
				}

				return true;
			});
		}
	});

	_manipulationActive = false;
	_selectionSystem.pivotChanged();

	// greebo: Deselect all faces if we are in brush and drag mode
	if (_selectionSystem.Mode() == SelectionSystem::ePrimitive && activeManipulator->getType() == selection::Manipulator::Drag)
	{
		SelectAllComponentWalker faceSelector(false, SelectionSystem::eFace);
		GlobalSceneGraph().root()->traverse(faceSelector);
	}

	if (_undoBegun)
	{
		// Cancel the undo operation, if one has been begun
		GlobalUndoSystem().cancel();
		_undoBegun = false;
	}

	// Update the views
	SceneChangeNotify();
}

bool ManipulateMouseTool::nothingSelected() const
{
	switch (_selectionSystem.Mode())
	{
	case SelectionSystem::eComponent:
		return _selectionSystem.countSelectedComponents() == 0;

	case SelectionSystem::eGroupPart:
	case SelectionSystem::ePrimitive:
	case SelectionSystem::eEntity:
		return _selectionSystem.countSelected() == 0;

	default:
		return false;
	};
}

void ManipulateMouseTool::renderOverlay()
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

void ManipulateMouseTool::render(RenderSystem& renderSystem, RenderableCollector& collector, const VolumeTest& volume)
{
#if 0
	if (nothingSelected()) return;

	const selection::ManipulatorPtr& activeManipulator = _selectionSystem.getActiveManipulator();

	if (!activeManipulator) return;

	if (!_pointShader)
	{
		_pointShader = renderSystem.capture("$POINT");
	}

	collector.setHighlightFlag(RenderableCollector::Highlight::Faces, false);
	collector.setHighlightFlag(RenderableCollector::Highlight::Primitives, false);

	collector.SetState(_pointShader, RenderableCollector::eWireframeOnly);
	collector.SetState(_pointShader, RenderableCollector::eFullMaterials);

	activeManipulator->render(collector, volume);
#endif
}

}
