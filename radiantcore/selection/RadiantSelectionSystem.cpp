#include "RadiantSelectionSystem.h"

#include "i18n.h"
#include "iundo.h"
#include "igrid.h"
#include "iselectiongroup.h"
#include "iradiant.h"
#include "ipreferencesystem.h"
#include "selection/SelectionPool.h"
#include "module/StaticModule.h"
#include "brush/csg/CSG.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Transformation.h"
#include "SceneWalkers.h"
#include "SelectionTestWalkers.h"
#include "command/ExecutionFailure.h"
#include "string/case_conv.h"
#include "messages/UnselectSelectionRequest.h"
#include "messages/ManipulatorModeToggleRequest.h"
#include "messages/ComponentSelectionModeToggleRequest.h"

#include "manipulators/DragManipulator.h"
#include "manipulators/ClipManipulator.h"
#include "manipulators/RotateManipulator.h"
#include "manipulators/TranslateManipulator.h"
#include "manipulators/ModelScaleManipulator.h"

#include <functional>

#include "SceneSelectionTesters.h"
#include "command/ExecutionNotPossible.h"

namespace selection
{

// --------- RadiantSelectionSystem Implementation ------------------------------------------

RadiantSelectionSystem::RadiantSelectionSystem() :
    _requestWorkZoneRecalculation(true),
    _defaultManipulatorType(IManipulator::Drag),
    _mode(SelectionMode::Primitive),
    _componentMode(ComponentSelectionMode::Default),
    _countPrimitive(0),
    _countComponent(0),
    _selectionFocusActive(false)
{}

const SelectionInfo& RadiantSelectionSystem::getSelectionInfo() {
    return _selectionInfo;
}

void RadiantSelectionSystem::addObserver(Observer* observer)
{
    if (observer != nullptr)
	{
        // Add the passed observer to the list
        _observers.insert(observer);
    }
}

void RadiantSelectionSystem::removeObserver(Observer* observer)
{
	_observers.erase(observer);
}

void RadiantSelectionSystem::notifyObservers(const scene::INodePtr& node, bool isComponent)
{
    // Cycle through the list of observers and call the moved method
    for (auto i = _observers.begin(); i != _observers.end(); )
	{
        (*i++)->selectionChanged(node, isComponent);
    }
}

void RadiantSelectionSystem::toggleSelectionFocus()
{
    if (_selectionFocusActive)
    {
        rMessage() << "Leaving selection focus mode" << std::endl;
        _selectionFocusActive = false;
        
        GlobalSceneGraph().root()->foreachNode([&](const scene::INodePtr& node)
        {
            node->setRenderState(scene::INode::RenderState::Active);
            return true;
        });

        // Re-select the pieces we had selected before entering
        for (const auto& node : _selectionFocusPool)
        {
            // Nodes might have been removed in the meantime
            if (node->inScene())
            {
                Node_setSelected(node, true);
            }
        }

        _selectionFocusPool.clear();
        SceneChangeNotify();
        return;
    }

    if (_selectionInfo.totalCount == 0)
    {
        throw cmd::ExecutionNotPossible("Nothing selected, cannot toggle selection focus mode");
    }

    _selectionFocusActive = true;
    _selectionFocusPool.clear();

    // Set the whole scene to inactive
    GlobalSceneGraph().root()->foreachNode([&](const scene::INodePtr& node)
    {
        node->setRenderState(scene::INode::RenderState::Inactive);
        return true;
    });

    // Extract the selection and set these nodes to active (including all child nodes)
    foreachSelected([&](const auto& node)
    {
        node->setRenderState(scene::INode::RenderState::Active);
        node->foreachNode([](const auto& child) { child->setRenderState(scene::INode::RenderState::Active); return true; });
        _selectionFocusPool.insert(node);
    });

    rMessage() << "Activated selection focus mode, got " << _selectionFocusPool.size() << " selectables in the pool" << std::endl;

    deselectAll();
    SceneChangeNotify();
}

bool RadiantSelectionSystem::selectionFocusIsActive()
{
    return _selectionFocusActive;
}

AABB RadiantSelectionSystem::getSelectionFocusBounds()
{
    AABB bounds;

    for (const auto& node : _selectionFocusPool)
    {
        if (node->inScene())
        {
            bounds.includeAABB(node->worldAABB());
        }
    }

    return bounds;
}

bool RadiantSelectionSystem::nodeCanBeSelectionTested(const scene::INodePtr& node)
{
    // All nodes pass if no focus is active, otherwise restrict to the pool
    if (!_selectionFocusActive || !node) return true;

    // Nodes can also be tested if their parent happens to be in the focus, otherwise
    // it'd be impossible to select focused func_static (their children are the ones that are tested)
    return _selectionFocusPool.count(node) > 0 || _selectionFocusPool.count(node->getParent()) > 0;
}

ISceneSelectionTester::Ptr RadiantSelectionSystem::createSceneSelectionTester(SelectionMode mode)
{
    auto nodePredicate = std::bind(&RadiantSelectionSystem::nodeCanBeSelectionTested, this, std::placeholders::_1);

    switch (mode)
    {
    case SelectionMode::Primitive:
        return std::make_shared<PrimitiveSelectionTester>(nodePredicate);
    case SelectionMode::Entity:
        return std::make_shared<EntitySelectionTester>(nodePredicate);
    case SelectionMode::GroupPart:
        return std::make_shared<GroupChildPrimitiveSelectionTester>(nodePredicate);
    case SelectionMode::MergeAction:
        return std::make_shared<MergeActionSelectionTester>(nodePredicate);
    case SelectionMode::Component:
        return std::make_shared<ComponentSelectionTester>(*this, nodePredicate);

    default:
        throw std::invalid_argument("Selection Mode not supported yet");
    }
}

void RadiantSelectionSystem::testSelectScene(SelectablesList& targetList, SelectionTest& test,
    const VolumeTest& view, SelectionMode mode)
{
    auto tester = createSceneSelectionTester(mode);
    tester->testSelectScene(view, test);

    tester->foreachSelectable([&](ISelectable* s) { targetList.push_back(s); });
}

bool RadiantSelectionSystem::nothingSelected() const
{
    return (getSelectionMode() == SelectionMode::Component && _countComponent == 0) ||
           (getSelectionMode() == SelectionMode::Primitive && _countPrimitive == 0) ||
           (getSelectionMode() == SelectionMode::GroupPart && _countPrimitive == 0);
}

void RadiantSelectionSystem::pivotChanged()
{
	_pivot.setNeedsRecalculation(true);
    SceneChangeNotify();
}

void RadiantSelectionSystem::pivotChangedSelection(const ISelectable& selectable) {
    pivotChanged();
}

bool RadiantSelectionSystem::higherEntitySelectionPriority() const
{
    return registry::getValue<bool>(RKEY_HIGHER_ENTITY_PRIORITY);
}

void RadiantSelectionSystem::setSelectionMode(SelectionMode mode)
{
    // Only change something if the mode has actually changed
    if (_mode != mode)
    {
        _mode = mode;
        pivotChanged();

        _sigSelectionModeChanged.emit(_mode);
    }
}

SelectionMode RadiantSelectionSystem::getSelectionMode() const
{
    return _mode;
}

// Set the current component mode to <mode>
void RadiantSelectionSystem::SetComponentMode(ComponentSelectionMode mode)
{
    if (_componentMode != mode)
    {
        _componentMode = mode;

        _sigComponentModeChanged.emit(_componentMode);
    }
}

// returns the current component mode
ComponentSelectionMode RadiantSelectionSystem::ComponentMode() const
{
    return _componentMode;
}

sigc::signal<void, SelectionMode>& RadiantSelectionSystem::signal_selectionModeChanged()
{
    return _sigSelectionModeChanged;
}

sigc::signal<void, ComponentSelectionMode>& RadiantSelectionSystem::signal_componentModeChanged()
{
    return _sigComponentModeChanged;
}

std::size_t RadiantSelectionSystem::registerManipulator(const ISceneManipulator::Ptr& manipulator)
{
	std::size_t newId = 1;

	while (_manipulators.count(newId) > 0)
	{
		++newId;

		if (newId == std::numeric_limits<std::size_t>::max())
		{
			throw std::runtime_error("Out of manipulator IDs");
		}
	}

	_manipulators.emplace(newId, manipulator);

	manipulator->setId(newId);

	if (!_activeManipulator)
	{
		_activeManipulator = manipulator;
	}

	return newId;
}

void RadiantSelectionSystem::unregisterManipulator(const ISceneManipulator::Ptr& manipulator)
{
	for (auto i = _manipulators.begin(); i != _manipulators.end(); ++i)
	{
		if (i->second == manipulator)
		{
			i->second->setId(0);
			_manipulators.erase(i);
			return;
		}
	}
}

IManipulator::Type RadiantSelectionSystem::getActiveManipulatorType()
{
	return _activeManipulator->getType();
}

const ISceneManipulator::Ptr& RadiantSelectionSystem::getActiveManipulator()
{
	return _activeManipulator;
}

void RadiantSelectionSystem::setActiveManipulator(std::size_t manipulatorId)
{
	auto found = _manipulators.find(manipulatorId);

	if (found == _manipulators.end())
	{
		rError() << "Cannot activate non-existent manipulator ID " << manipulatorId << std::endl;
		return;
	}

    // Remove any visuals from the previous manipulator
    if (_activeManipulator)
    {
        _activeManipulator->clearRenderables();
    }

	_activeManipulator = found->second;

	// Release the user lock when switching manipulators
	_pivot.setUserLocked(false);

	pivotChanged();
}

void RadiantSelectionSystem::setActiveManipulator(IManipulator::Type manipulatorType)
{
	for (const auto& [_, manipulator] : _manipulators)
	{
		if (manipulator->getType() == manipulatorType)
		{
			_activeManipulator = manipulator;

			// Release the user lock when switching manipulators
			_pivot.setUserLocked(false);

			pivotChanged();
			return;
		}
	}

	rError() << "Cannot activate non-existent manipulator by type " << manipulatorType << std::endl;
}

sigc::signal<void, selection::IManipulator::Type>& RadiantSelectionSystem::signal_activeManipulatorChanged()
{
    return _sigActiveManipulatorChanged;
}

// return the number of selected primitives
std::size_t RadiantSelectionSystem::countSelected() const {
    return _countPrimitive;
}

// return the number of selected components
std::size_t RadiantSelectionSystem::countSelectedComponents() const {
    return _countComponent;
}

// This is called if the selection changes, so that the local list of selected nodes can be updated
void RadiantSelectionSystem::onSelectedChanged(const scene::INodePtr& node, const ISelectable& selectable)
{
    // Cache the selection state
    bool isSelected = selectable.isSelected();
    int delta = isSelected ? +1 : -1;

    _countPrimitive += delta;

    _selectionInfo.totalCount += delta;

    if (Node_isPatch(node))
	{
        _selectionInfo.patchCount += delta;
    }
    else if (Node_isBrush(node))
	{
        _selectionInfo.brushCount += delta;
    }
    else
	{
        _selectionInfo.entityCount += delta;
    }

    // If the selectable is selected, add it to the local selection list, otherwise remove it
    if (isSelected)
	{
        _selection.append(node);

        // Any selectable that is not in the pool yet will be added
        // otherwise creating new nodes is making them unselectable
        if (_selectionFocusActive)
        {
            _selectionFocusPool.insert(node);
        }
    }
    else
	{
        _selection.erase(node);
    }

	// greebo: Moved this here, the selectionInfo structure should be up to date before calling this
	_sigSelectionChanged(selectable);

    // Notify observers, FALSE = primitive selection change
    notifyObservers(node, false);

    // Check if the number of selected primitives in the list matches the value of the selection counter
    ASSERT_MESSAGE(_selection.size() == _countPrimitive, "selection-tracking error");

    _requestWorkZoneRecalculation = true;

	// When everything is deselected, release the pivot user lock
	if (_selection.empty())
	{
		_pivot.setUserLocked(false);
	}
}

// greebo: This should be called "onComponentSelectionChanged", as it is a similar function of the above one
// Updates the internal list of component nodes if the component selection gets changed
void RadiantSelectionSystem::onComponentSelection(const scene::INodePtr& node, const ISelectable& selectable)
{
    int delta = selectable.isSelected() ? +1 : -1;

    _countComponent += delta;

    _selectionInfo.totalCount += delta;
    _selectionInfo.componentCount += delta;

    // If the instance got selected, add it to the list, otherwise remove it
    if (selectable.isSelected()) {
        _componentSelection.append(node);
    }
    else {
        _componentSelection.erase(node);
    }

	// Moved here, since the _selectionInfo struct needs to be up to date
	_sigSelectionChanged(selectable);

    // Notify observers, TRUE => this is a component selection change
    notifyObservers(node, true);

    // Check if the number of selected components in the list matches the value of the selection counter
    ASSERT_MESSAGE(_componentSelection.size() == _countComponent, "component selection-tracking error");

    _requestWorkZoneRecalculation = true;

	if (_componentSelection.empty())
	{
		_pivot.setUserLocked(false);
	}
}

// Returns the last instance in the list (if the list is not empty)
scene::INodePtr RadiantSelectionSystem::ultimateSelected()
{
    ASSERT_MESSAGE(_selection.size() > 0, "no instance selected");
    return _selection.ultimate();
}

// Returns the instance before the last instance in the list (second from the end)
scene::INodePtr RadiantSelectionSystem::penultimateSelected()
{
    ASSERT_MESSAGE(_selection.size() > 1, "only one instance selected");
    return _selection.penultimate();
}

// Deselect or select all the instances in the scenegraph and notify the manipulator class as well
void RadiantSelectionSystem::setSelectedAll(bool selected)
{
	GlobalSceneGraph().foreachNode([&] (const scene::INodePtr& node)->bool
	{
		Node_setSelected(node, selected);
		return true;
	});

    _activeManipulator->setSelected(selected);
}

// Deselect or select all the component instances in the scenegraph and notify the manipulator class as well
void RadiantSelectionSystem::setSelectedAllComponents(bool selected)
{
	const scene::INodePtr& root = GlobalSceneGraph().root();

	if (root)
	{
		// Select all components in the scene, be it vertices, edges or faces
		root->foreachNode([&] (const scene::INodePtr& node)->bool
		{
			ComponentSelectionTestablePtr componentSelectionTestable = Node_getComponentSelectionTestable(node);

			if (componentSelectionTestable)
			{
				componentSelectionTestable->setSelectedComponents(selected, ComponentSelectionMode::Vertex);
				componentSelectionTestable->setSelectedComponents(selected, ComponentSelectionMode::Edge);
				componentSelectionTestable->setSelectedComponents(selected, ComponentSelectionMode::Face);
			}

			return true;
		});
	}

	_activeManipulator->setSelected(selected);
}

// Traverse the current selection components and visit them with the given visitor class
void RadiantSelectionSystem::foreachSelectedComponent(const Visitor& visitor)
{
    for (auto i = _componentSelection.begin(); i != _componentSelection.end(); /* in-loop increment */)
    {
        visitor.visit((i++)->first);
    }
}

void RadiantSelectionSystem::foreachSelected(const std::function<void(const scene::INodePtr&)>& functor)
{
	for (auto i = _selection.begin(); i != _selection.end(); /* in-loop increment */)
    {
        functor((i++)->first);
    }
}

void RadiantSelectionSystem::foreachSelectedComponent(const std::function<void(const scene::INodePtr&)>& functor)
{
	for (auto i = _componentSelection.begin(); i != _componentSelection.end(); /* in-loop increment */)
    {
        functor((i++)->first);
    }
}

void RadiantSelectionSystem::foreachBrush(const std::function<void(Brush&)>& functor)
{
	BrushSelectionWalker walker(functor);

	for (auto i = _selection.begin(); i != _selection.end(); /* in-loop increment */)
    {
		walker.visit((i++)->first); // Handles group nodes recursively
    }
}

void RadiantSelectionSystem::foreachFace(const std::function<void(IFace&)>& functor)
{
	FaceSelectionWalker walker(functor);

	for (auto i = _selection.begin(); i != _selection.end(); /* in-loop increment */)
    {
		walker.visit((i++)->first); // Handles group nodes recursively
    }

	// Handle the component selection too
	algorithm::forEachSelectedFaceComponent(functor);
}

void RadiantSelectionSystem::foreachPatch(const std::function<void(IPatch&)>& functor)
{
	PatchSelectionWalker walker(functor);

	for (auto i = _selection.begin(); i != _selection.end(); /* in-loop increment */)
    {
		walker.visit((i++)->first); // Handles group nodes recursively
    }
}

std::size_t RadiantSelectionSystem::getSelectedFaceCount()
{
    return FaceInstance::Selection().size();
}

IFace& RadiantSelectionSystem::getSingleSelectedFace()
{
    if (getSelectedFaceCount() == 1)
    {
        return FaceInstance::Selection().back()->getFace();
    }
    else
    {
        throw cmd::ExecutionFailure(string::to_string(getSelectedFaceCount()));
    }
}

// Hub function for "deselect all", this passes the deselect call to the according functions
void RadiantSelectionSystem::deselectAll() {
    if (getSelectionMode() == SelectionMode::Component) {
        setSelectedAllComponents(false);
    }
    else {
        setSelectedAll(false);
    }
}

void RadiantSelectionSystem::selectPoint(SelectionTest& test, EModifier modifier, bool face)
{
    // If the user is holding the replace modifiers (default: Alt-Shift), deselect the current selection
    if (modifier == SelectionSystem::eReplace) {
        if (face) {
            setSelectedAllComponents(false);
        }
        else {
            deselectAll();
        }
    }

    // The possible candidates are stored in the SelectablesSet
    SelectablesList candidates;

    if (face)
    {
        SelectionPool selector;

        ComponentSelector tester(selector, test, ComponentSelectionMode::Face);
        GlobalSceneGraph().foreachVisibleNodeInVolume(test.getVolume(), [&](const scene::INodePtr& node)
        {
            if (nodeCanBeSelectionTested(node))
            {
                tester.testNode(node);
            }
            return true;
        });

        // Load them all into the vector
        for (auto i = selector.begin(); i != selector.end(); ++i)
        {
            candidates.push_back(i->second);
        }
    }
    else {
        testSelectScene(candidates, test, test.getVolume(), getSelectionMode());
    }

    // Was the selection test successful (have we found anything to select)?
    performPointSelection(candidates, modifier);

    onSelectionPerformed();
}

void RadiantSelectionSystem::setSelectionStatus(ISelectable* selectable, bool selected)
{
    // In focus mode the selection is not propagated to group peers
    if (_selectionFocusActive)
    {
        selectable->setSelected(selected);
        return;
    }

	if (auto groupSelectable = dynamic_cast<IGroupSelectable*>(selectable); groupSelectable)
	{
		groupSelectable->setSelected(selected, true); // propagate selection state to group peers
	}
	else
	{
		selectable->setSelected(selected);
	}
}

void RadiantSelectionSystem::performPointSelection(const SelectablesList& candidates, EModifier modifier)
{
	if (candidates.empty()) return;

	// Yes, now determine how we should interpret the click
	switch (modifier)
	{
		// If we are in toggle mode (Shift-Left-Click by default), just toggle the
		// selection of the "topmost" item
		case eToggle:
		{
			ISelectable* best = candidates.front();
			// toggle selection of the object with least depth (=first in the list)
			setSelectionStatus(best, !best->isSelected());
		}
		break;

		// greebo: eReplace mode gets active as soon as the user holds the replace modifiers down
		// and clicks (by default: Alt-Shift). eReplace is only active during the first click
		// afterwards we are in cycle mode.
		// if cycle mode not enabled, enable it
		case eReplace:
		{
			// select closest (=first in the list)
			setSelectionStatus(candidates.front(), true);
		}
		break;

		// select the next object in the list from the one already selected
		// greebo: eCycle is set if the user keeps holding the replace modifiers (Alt-Shift)
		// and does NOT move the mouse between the clicks, otherwise we fall back into eReplace mode
		// Note: The mode is set in SelectObserver::testSelect()
		case eCycle:
		{
			// Cycle through the selection pool and activate the item right after the currently selected
			SelectablesList::const_iterator i = candidates.begin();

			while (i != candidates.end())
			{
				if ((*i)->isSelected())
				{
					// unselect the currently selected one
					setSelectionStatus(*i, false);

					// check if there is a "next" item in the list, if not: select the first item
					++i;

					if (i != candidates.end())
					{
						setSelectionStatus(*i, true);
					}
					else
					{
						setSelectionStatus(candidates.front(), true);
					}
					break;
				}

				++i;
			}
		}
		break;

		default:
			break;
	};
}

void RadiantSelectionSystem::selectArea(SelectionTest& test, SelectionSystem::EModifier modifier, bool face)
{
    // If we are in replace mode, deselect all the components or previous selections
    if (modifier == SelectionSystem::eReplace)
    {
        if (face)
        {
            setSelectedAllComponents(false);
        }
        else
        {
            deselectAll();
        }
    }

    // The posssible candidates go here
    SelectionPool pool;

    SelectablesList candidates;

    if (face)
    {
        ComponentSelector tester(pool, test, ComponentSelectionMode::Face);
        GlobalSceneGraph().foreachVisibleNodeInVolume(test.getVolume(), [&](const scene::INodePtr& node)
        {
            if (nodeCanBeSelectionTested(node))
            {
                tester.testNode(node);
            }
            return true;
        });

        // Load them all into the vector
        for (auto i = pool.begin(); i != pool.end(); ++i)
        {
            candidates.push_back(i->second);
        }
    }
    else
    {
        testSelectScene(candidates, test, test.getVolume(), getSelectionMode());
    }

    // Since toggling a selectable might trigger a group-selection
    // we need to keep track of the desired state of each selectable
    typedef std::map<ISelectable*, bool> SelectablesMap;
    SelectablesMap selectableStates;

    for (ISelectable* selectable : candidates)
    {
        bool desiredState = !(modifier == SelectionSystem::eToggle && selectable->isSelected());
        selectableStates.insert(SelectablesMap::value_type(selectable, desiredState));
    }

    for (const auto& state : selectableStates)
    {
        setSelectionStatus(state.first, state.second);
    }

    onSelectionPerformed();
}

void RadiantSelectionSystem::onSelectionPerformed()
{
    // #5460: To fix the workzone not being recalculated when the selection changes,
    // invoke getWorkZone() here. Since the core binary doesn't have any idle processing
    // anymore, we need to recalculate the workzone after the user is done selecting
    getWorkZone();
}

void RadiantSelectionSystem::onManipulationStart()
{
	// Save the pivot state now that the transformation is starting
	_pivot.beginOperation();
}

void RadiantSelectionSystem::onManipulationChanged()
{
	_requestWorkZoneRecalculation = true;

	GlobalSceneGraph().sceneChanged();
}

void RadiantSelectionSystem::onManipulationEnd()
{
    GlobalSceneGraph().foreachNode(scene::freezeTransformableNode);

    _pivot.endOperation();

	// The selection bounds have possibly changed
	_requestWorkZoneRecalculation = true;

    const auto& activeManipulator = getActiveManipulator();
    assert(activeManipulator);

    // greebo: Deselect all faces if we are in brush and drag mode
    if ((getSelectionMode() == SelectionMode::Primitive || getSelectionMode() == SelectionMode::GroupPart) &&
        activeManipulator->getType() == IManipulator::Drag)
    {
        SelectAllComponentWalker faceSelector(false, ComponentSelectionMode::Face);
        GlobalSceneGraph().root()->traverse(faceSelector);
    }

    {
        // Remove all degenerated brushes from the scene graph (should emit a warning)
        // Do this in an undoable transaction we cannot be sure that one is active at this point
        UndoableCommand cmd(_("Degenerate Brushes removed"));
        SelectionSystem::foreachSelected(RemoveDegenerateBrushWalker());
    }

    pivotChanged();
    activeManipulator->setSelected(false);

    GlobalSceneGraph().sceneChanged();
}

void RadiantSelectionSystem::onManipulationCancelled()
{
    const auto& activeManipulator = getActiveManipulator();
    assert(activeManipulator);

    // Unselect any currently selected manipulators to be sure
    activeManipulator->setSelected(false);

    // Tell all the scene objects to revert their transformations
    foreachSelected([](const scene::INodePtr& node)
    {
        ITransformablePtr transform = scene::node_cast<ITransformable>(node);

        if (transform)
        {
            transform->revertTransform();
        }

        // In case of entities, we need to inform the child nodes as well
        if (Node_getEntity(node))
        {
            node->foreachNode([&](const scene::INodePtr& child)
            {
                ITransformablePtr transform = scene::node_cast<ITransformable>(child);

                if (transform)
                {
                    transform->revertTransform();
                }

                return true;
            });
        }
    });

    // greebo: Deselect all faces if we are in brush and drag mode
    if (getSelectionMode() == SelectionMode::Primitive && activeManipulator->getType() == IManipulator::Drag)
    {
        SelectAllComponentWalker faceSelector(false, ComponentSelectionMode::Face);
        GlobalSceneGraph().root()->traverse(faceSelector);
    }

    _pivot.cancelOperation();

    pivotChanged();
}

const Matrix4& RadiantSelectionSystem::getPivot2World()
{
    return _pivot.getMatrix4();
}

const WorkZone& RadiantSelectionSystem::getWorkZone()
{
    // Check if we should recalculate the workzone
    if (_requestWorkZoneRecalculation)
    {
        _requestWorkZoneRecalculation = false;

        // When no items are selected, leave a (valid) workzone alone to allow
        // for creation of new elements within the bounds of a previous selection
        if (_selectionInfo.totalCount > 0 || !_workZone.bounds.isValid())
        {
            // Recalculate the workzone based on the current selection
            AABB bounds = algorithm::getCurrentSelectionBounds();

            if (bounds.isValid())
            {
                _workZone.max = bounds.origin + bounds.extents;
                _workZone.min = bounds.origin - bounds.extents;
            }
            else
            {
                // A zero-sized workzone doesn't make much sense, set to default
                _workZone.max = Vector3(64, 64, 64);
                _workZone.min = Vector3(-64, -64, -64);
            }

            _workZone.bounds = bounds;
        }
    }

    return _workZone;
}

Vector3 RadiantSelectionSystem::getCurrentSelectionCenter()
{
    return algorithm::getCurrentSelectionCenter();
}

void RadiantSelectionSystem::onPreRender(const VolumeTest& volume)
{
    if (!nothingSelected())
    {
        auto renderSystem = GlobalMapModule().getRoot()->getRenderSystem();

        if (renderSystem)
        {
            _activeManipulator->onPreRender(renderSystem, volume);
        }
        else
        {
            _activeManipulator->clearRenderables();
        }
    }
    else
    {
        _activeManipulator->clearRenderables();
    }
}

void RadiantSelectionSystem::onSceneBoundsChanged()
{
    // The bounds of the scenegraph have (possibly) changed
    pivotChanged();

    _requestWorkZoneRecalculation = true;
}

const std::string& RadiantSelectionSystem::getName() const
{
    static std::string _name(MODULE_SELECTIONSYSTEM);
    return _name;
}

const StringSet& RadiantSelectionSystem::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
	{
        _dependencies.insert(MODULE_RENDERSYSTEM);
        _dependencies.insert(MODULE_XMLREGISTRY);
        _dependencies.insert(MODULE_GRID);
        _dependencies.insert(MODULE_SCENEGRAPH);
		_dependencies.insert(MODULE_MAP);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_OPENGL);
    }

    return _dependencies;
}

void RadiantSelectionSystem::initialiseModule(const IApplicationContext& ctx)
{
	_pivot.initialise();

	// Add manipulators
	registerManipulator(std::make_shared<DragManipulator>(_pivot, *this, *this));
	registerManipulator(std::make_shared<ClipManipulator>());
	registerManipulator(std::make_shared<TranslateManipulator>(_pivot, 2, 64.0f));
	registerManipulator(std::make_shared<RotateManipulator>(_pivot, 8, 64.0f));
	registerManipulator(std::make_shared<ModelScaleManipulator>(_pivot));

	_defaultManipulatorType = IManipulator::Drag;
	setActiveManipulator(_defaultManipulatorType);
    pivotChanged();

    _sigSelectionChanged.connect(
        sigc::mem_fun(this, &RadiantSelectionSystem::pivotChangedSelection)
    );

	_sigSelectionChanged.connect(
		sigc::mem_fun(this, &RadiantSelectionSystem::checkComponentModeSelectionMode)
    );

    GlobalGrid().signal_gridChanged().connect(
        sigc::mem_fun(this, &RadiantSelectionSystem::pivotChanged)
    );

    GlobalCommandSystem().addCommand("ToggleManipulatorMode",
        std::bind(&RadiantSelectionSystem::toggleManipulatorModeCmd, this, std::placeholders::_1), { cmd::ARGTYPE_STRING });

    GlobalCommandSystem().addCommand("ToggleEntitySelectionMode", std::bind(&RadiantSelectionSystem::toggleEntityMode, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("ToggleGroupPartSelectionMode", std::bind(&RadiantSelectionSystem::toggleGroupPartMode, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("ToggleMergeActionSelectionMode", std::bind(&RadiantSelectionSystem::toggleMergeActionMode, this, std::placeholders::_1));

    GlobalCommandSystem().addWithCheck("ToggleSelectionFocus",
        [this](const auto&) { toggleSelectionFocus(); },
        [this]() { return _selectionFocusActive || _selectionInfo.totalCount > 0; });

    GlobalCommandSystem().addCommand("ToggleComponentSelectionMode",
        std::bind(&RadiantSelectionSystem::toggleComponentModeCmd, this, std::placeholders::_1), { cmd::ARGTYPE_STRING });

    selection::algorithm::registerCommands();
    brush::algorithm::registerCommands();

	GlobalCommandSystem().addCommand("UnSelectSelection", std::bind(&RadiantSelectionSystem::deselectCmd, this, std::placeholders::_1));

    GlobalCommandSystem().addCommand("RotateSelectedEulerXYZ", selection::algorithm::rotateSelectedEulerXYZ, { cmd::ARGTYPE_VECTOR3 });
    GlobalCommandSystem().addCommand("ScaleSelected", selection::algorithm::scaleSelectedCmd, { cmd::ARGTYPE_VECTOR3 });

	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Selection"));

	page.appendCheckBox(_("Ignore light volume bounds when calculating default rotation pivot location"),
		SceneManipulationPivot::RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES);

    // Connect the bounds changed caller
    GlobalSceneGraph().signal_boundsChanged().connect(
        sigc::mem_fun(this, &RadiantSelectionSystem::onSceneBoundsChanged)
    );

    GlobalRenderSystem().attachRenderable(*this);

	GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &RadiantSelectionSystem::onMapEvent));
}

void RadiantSelectionSystem::shutdownModule()
{
    _selectionFocusPool.clear();

    // greebo: Unselect everything so that no references to scene::Nodes
    // are kept after shutdown, causing destruction issues.
    setSelectedAll(false);
    setSelectedAllComponents(false);

	// In pathological cases this list might contain remnants. First, give each
	// selectable node a chance to remove itself from the container by setting
	// its own selected state to false (rather than waiting for this to happen
	// in its destructor).
    for (auto i = _selection.begin(); i != _selection.end(); )
    {
        // Take a reference to the node and increment the iterator while the
        // iterator is still valid.
        scene::INodePtr node = (i++)->first;

        // If this is a selectable node, unselect it (which might remove it from
        // the map and invalidate the original iterator)
        auto selectable = scene::node_cast<ISelectable>(node);
        if (selectable)
            selectable->setSelected(false);
    }

    // Clear the list of anything which remains.
	_selection.clear();

	_activeManipulator.reset();
	_manipulators.clear();

    GlobalRenderSystem().detachRenderable(*this);
}

void RadiantSelectionSystem::checkComponentModeSelectionMode(const ISelectable& selectable)
{
	// This seems to be a fail-safe method, to detect situations where component mode is still
	// active without any primitive selected - in which case the method exits component mode.
	if (getSelectionMode() == SelectionMode::Component && countSelected() == 0)
	{
		activateDefaultMode();
		onComponentModeChanged();
	}
}

std::size_t RadiantSelectionSystem::getManipulatorIdForType(IManipulator::Type type)
{
	for (const auto& [id, manipulator] : _manipulators)
	{
		if (manipulator->getType() == type)
		{
			return id;
		}
	}

	return 0;
}

void RadiantSelectionSystem::toggleManipulatorModeById(std::size_t manipId)
{
	std::size_t defaultManipId = getManipulatorIdForType(_defaultManipulatorType);

	if (defaultManipId == 0)
	{
		return;
	}

	// Switch back to the default mode if we're already in <mode>
	if (_activeManipulator->getId() == manipId && defaultManipId != manipId)
	{
		toggleManipulatorModeById(defaultManipId);
	}
	else // we're not in <mode> yet
	{
		std::size_t clipperId = getManipulatorIdForType(IManipulator::Clip);

		if (manipId == clipperId)
		{
			activateDefaultMode();
			GlobalClipper().onClipMode(true);
		}
		else
		{
			GlobalClipper().onClipMode(false);
		}

		setActiveManipulator(manipId);
		onManipulatorModeChanged();
		onComponentModeChanged();
	}
}

void RadiantSelectionSystem::toggleManipulatorModeCmd(const cmd::ArgumentList& args)
{
    if (args.size() != 1)
    {
        rWarning() << "Usage: ToggleManipulatorMode <manipulator>" << std::endl;
        rWarning() << " with <manipulator> being one of the following: " << std::endl;
        rWarning() << "      Drag" << std::endl;
        rWarning() << "      Translate" << std::endl;
        rWarning() << "      Rotate" << std::endl;
        rWarning() << "      Clip" << std::endl;
        rWarning() << "      ModelScale" << std::endl;
        return;
    }

    auto manip = string::to_lower_copy(args[0].getString());
    IManipulator::Type type;

    if (manip == "drag")
    {
        type = IManipulator::Drag;
    }
    else if (manip == "translate")
    {
        type = IManipulator::Translate;
    }
    else if (manip == "rotate")
    {
        type = IManipulator::Rotate;
    }
    else if (manip == "clip")
    {
        type = IManipulator::Clip;
    }
    else if (manip == "modelscale")
    {
        type = IManipulator::ModelScale;
    }
    else
    {
        rError() << "Unknown manipulator type: " << manip << std::endl;
        return;
    }

    // Send out the event for the other views like the texture tool
    ManipulatorModeToggleRequest request(type);
    GlobalRadiantCore().getMessageBus().sendMessage(request);

    if (!request.isHandled())
    {
        // Handle it ourselves
        toggleManipulatorModeById(getManipulatorIdForType(type));
    }
}

void RadiantSelectionSystem::toggleManipulatorMode(IManipulator::Type type)
{
	// Switch back to the default mode if we're already in <mode>
	if (_activeManipulator->getType() == type && _defaultManipulatorType != type)
	{
		toggleManipulatorMode(_defaultManipulatorType);
	}
	else // we're not in <mode> yet
	{
		if (type == IManipulator::Clip)
		{
			activateDefaultMode();
			GlobalClipper().onClipMode(true);
		}
		else
		{
			GlobalClipper().onClipMode(false);
		}

		setActiveManipulator(type);
		onManipulatorModeChanged();
		onComponentModeChanged();
	}
}

void RadiantSelectionSystem::activateDefaultMode()
{
	setSelectionMode(SelectionMode::Primitive);
	SetComponentMode(ComponentSelectionMode::Default);

	SceneChangeNotify();
}

void RadiantSelectionSystem::toggleComponentMode(ComponentSelectionMode mode)
{
	if (getSelectionMode() == SelectionMode::Component && ComponentMode() == mode)
	{
		// De-select all the selected components before switching back
		setSelectedAllComponents(false);
		activateDefaultMode();
	}
	else if (countSelected() != 0)
	{
		if (!_activeManipulator->supportsComponentManipulation())
		{
			toggleManipulatorMode(_defaultManipulatorType);
		}

		setSelectionMode(SelectionMode::Component);
		SetComponentMode(mode);
	}

	onComponentModeChanged();
}

void RadiantSelectionSystem::toggleComponentModeCmd(const cmd::ArgumentList& args)
{
    if (args.size() != 1)
    {
        rWarning() << "Usage: ToggleComponentSelectionMode <mode>" << std::endl;
        rWarning() << " with <mode> being one of the following: " << std::endl;
        rWarning() << "      Default" << std::endl;
        rWarning() << "      Vertex" << std::endl;
        rWarning() << "      Edge" << std::endl;
        rWarning() << "      Face" << std::endl;
        return;
    }

    auto modeStr = string::to_lower_copy(args[0].getString());
    ComponentSelectionMode mode;

    if (modeStr == "vertex")
    {
        mode = ComponentSelectionMode::Vertex;
    }
    else if (modeStr == "edge")
    {
        mode = ComponentSelectionMode::Edge;
    }
    else if (modeStr == "face")
    {
        mode = ComponentSelectionMode::Face;
    }
    else if (modeStr == "default")
    {
        mode = ComponentSelectionMode::Default;
    }

    ComponentSelectionModeToggleRequest request(mode);
    GlobalRadiantCore().getMessageBus().sendMessage(request);

    if (!request.isHandled())
    {
        // Handle it ourselves
        toggleComponentMode(mode);
    }
}

void RadiantSelectionSystem::toggleEntityMode(const cmd::ArgumentList& args)
{
	if (getSelectionMode() == SelectionMode::Entity)
	{
		activateDefaultMode();
	}
	else
	{
		setSelectionMode(SelectionMode::Entity);
		SetComponentMode(ComponentSelectionMode::Default);
	}

	onManipulatorModeChanged();
	onComponentModeChanged();
}

void RadiantSelectionSystem::toggleGroupPartMode(const cmd::ArgumentList& args)
{
	if (getSelectionMode() == SelectionMode::GroupPart)
	{
		activateDefaultMode();
	}
	else
	{
		// De-select everything when switching to group part mode
        setSelectedAllComponents(false);

        // greebo: Instead of de-selecting everything, check if we can
        // transform existing selections into something useful

        // Collect all entities containing child primitives
        std::vector<scene::INodePtr> groupEntityNodes;
        foreachSelected([&](const scene::INodePtr& node)
        {
            if (scene::hasChildPrimitives(node))
            {
                groupEntityNodes.push_back(node);
            }
        });

        // Now deselect everything and select all child primitives instead
        setSelectedAll(false);

        std::for_each(groupEntityNodes.begin(), groupEntityNodes.end(), [&](const scene::INodePtr& node)
        {
            node->foreachNode([&] (const scene::INodePtr& child)->bool
            {
                Node_setSelected(child, true);
                return true;
            });
        });

		setSelectionMode(SelectionMode::GroupPart);
		SetComponentMode(ComponentSelectionMode::Default);
	}

	onManipulatorModeChanged();
	onComponentModeChanged();
}

void RadiantSelectionSystem::toggleMergeActionMode(const cmd::ArgumentList& args)
{
    auto oldMode = getSelectionMode();

    if (getSelectionMode() == SelectionMode::MergeAction)
    {
        activateDefaultMode();
    }
    // Only allow switching if the map has an active merge operation
    else if (GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
    {
        // De-select everything when switching to merge mode
        setSelectedAll(false);
        setSelectedAllComponents(false);

        setSelectionMode(SelectionMode::MergeAction);
        SetComponentMode(ComponentSelectionMode::Default);
    }

    if (oldMode != getSelectionMode())
    {
        onManipulatorModeChanged();
        onComponentModeChanged();
    }
}

void RadiantSelectionSystem::toggleSelectionFocus(const cmd::ArgumentList& args)
{
    toggleSelectionFocus();
}

void RadiantSelectionSystem::onManipulatorModeChanged()
{
    _sigActiveManipulatorChanged.emit(getActiveManipulatorType());
	SceneChangeNotify();
}

void RadiantSelectionSystem::onComponentModeChanged()
{
	SceneChangeNotify();
}

// called when the escape key is used (either on the main window or on an inspector)
void RadiantSelectionSystem::deselectCmd(const cmd::ArgumentList& args)
{
    // First send out the request to let other systems like the texture tool
    // react to the deselection event
    UnselectSelectionRequest request;
    GlobalRadiantCore().getMessageBus().sendMessage(request);

    if (request.isHandled())
    {
        return; // already handled by other systems
    }

    if (getSelectionMode() == SelectionMode::Component)
	{
		if (countSelectedComponents() != 0)
		{
			setSelectedAllComponents(false);
		}
		else
		{
			activateDefaultMode();
			onComponentModeChanged();
		}
	}
	else
	{
		if (countSelectedComponents() != 0)
		{
			setSelectedAllComponents(false);
		}
		else
		{
            if (countSelected() > 0)
            {
                setSelectedAll(false);
            }
            // With no selection left, hitting ESC will leave focus mode
            else if (_selectionFocusActive)
            {
                toggleSelectionFocus();
            }
		}
	}
}

void RadiantSelectionSystem::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloading || ev == IMap::MapLoading)
	{
        _selectionFocusActive = false;
        _selectionFocusPool.clear();
		setSelectedAll(false);
		setSelectedAllComponents(false);
	}
}

// Define the static SelectionSystem module
module::StaticModuleRegistration<RadiantSelectionSystem> radiantSelectionSystemModule;

}
