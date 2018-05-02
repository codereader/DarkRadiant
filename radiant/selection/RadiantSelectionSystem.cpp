#include "RadiantSelectionSystem.h"

#include "i18n.h"
#include "iundo.h"
#include "igrid.h"
#include "iselectiongroup.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"
#include "imousetoolmanager.h"
#include "SelectionPool.h"
#include "SelectionTest.h"
#include "modulesystem/StaticModule.h"
#include "SelectionMouseTools.h"
#include "ManipulateMouseTool.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Primitives.h"
#include "xyview/GlobalXYWnd.h"
#include "SceneWalkers.h"

#include "manipulators/DragManipulator.h"
#include "manipulators/ClipManipulator.h"
#include "manipulators/RotateManipulator.h"
#include "manipulators/ScaleManipulator.h"
#include "manipulators/TranslateManipulator.h"
#include "manipulators/ModelScaleManipulator.h"

#include <functional>

namespace selection
{

// --------- RadiantSelectionSystem Implementation ------------------------------------------

RadiantSelectionSystem::RadiantSelectionSystem() :
    _requestSceneGraphChange(false),
    _requestWorkZoneRecalculation(true),
    _defaultManipulatorType(Manipulator::Drag),
    _mode(ePrimitive),
    _componentMode(eDefault),
    _countPrimitive(0),
    _countComponent(0)
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
    for (ObserverList::iterator i = _observers.begin(); i != _observers.end(); )
	{
        (*i++)->selectionChanged(node, isComponent);
    }
}

void RadiantSelectionSystem::testSelectScene(SelectablesList& targetList, SelectionTest& test,
                                             const render::View& view, SelectionSystem::EMode mode,
                                             SelectionSystem::EComponentMode componentMode)
{
    // The (temporary) storage pool
    SelectionPool selector;
    SelectionPool sel2;

    switch(mode)
    {
        case eEntity:
        {
            // Instantiate a walker class which is specialised for selecting entities
            EntitySelector entityTester(selector, test);
            GlobalSceneGraph().foreachVisibleNodeInVolume(view, entityTester);

            for (SelectionPool::const_iterator i = selector.begin(); i != selector.end(); ++i)
            {
                targetList.push_back(i->second);
            }
        }
        break;

        case ePrimitive:
        {
            // Do we have a camera view (filled rendering?)
            if (view.fill() || !GlobalXYWnd().higherEntitySelectionPriority())
            {
                // Test for any visible elements (primitives, entities), but don't select child primitives
                AnySelector anyTester(selector, test);
                GlobalSceneGraph().foreachVisibleNodeInVolume(view, anyTester);
            }
            else
            {
                // We have an orthoview, here, select entities first

                // First, obtain all the selectable entities
                EntitySelector entityTester(selector, test);
                GlobalSceneGraph().foreachVisibleNodeInVolume(view, entityTester);

                // Now retrieve all the selectable primitives
                PrimitiveSelector primitiveTester(sel2, test);
                GlobalSceneGraph().foreachVisibleNodeInVolume(view, primitiveTester);
            }

            // Add the first selection crop to the target vector
            for (SelectionPool::const_iterator i = selector.begin(); i != selector.end(); ++i) {
                targetList.push_back(i->second);
            }

            // Add the secondary crop to the vector (if it has any entries)
            for (SelectionPool::const_iterator i = sel2.begin(); i != sel2.end(); ++i) {
                // Check for duplicates
                SelectablesList::iterator j;
                for (j = targetList.begin(); j != targetList.end(); ++j) {
                    if (*j == i->second) break;
                }
                // Insert if not yet in the list
                if (j == targetList.end()) {
                    targetList.push_back(i->second);
                }
            }
        }
        break;

        case eGroupPart:
        {
            // Retrieve all the selectable primitives of group nodes
            GroupChildPrimitiveSelector primitiveTester(selector, test);
            GlobalSceneGraph().foreachVisibleNodeInVolume(view, primitiveTester);

            // Add the selection crop to the target vector
            for (SelectionPool::const_iterator i = selector.begin(); i != selector.end(); ++i)
            {
                targetList.push_back(i->second);
            }
        }
        break;

        case eComponent:
        {
            ComponentSelector selectionTester(selector, test, componentMode);
            foreachSelected(selectionTester);

            for (SelectionPool::const_iterator i = selector.begin(); i != selector.end(); ++i)
            {
                targetList.push_back(i->second);
            }
        }
        break;
    } // switch
}

/* greebo: This is true if nothing is selected (either in component mode or in primitive mode)
 */
bool RadiantSelectionSystem::nothingSelected() const
{
    return (Mode() == eComponent && _countComponent == 0) ||
           (Mode() == ePrimitive && _countPrimitive == 0) ||
           (Mode() == eGroupPart && _countPrimitive == 0);
}

void RadiantSelectionSystem::pivotChanged()  
{
	_pivot.setNeedsRecalculation(true);
    SceneChangeNotify();
}

void RadiantSelectionSystem::pivotChangedSelection(const ISelectable& selectable) {
    pivotChanged();
}

// Sets the current selection mode (Entity, Component or Primitive)
void RadiantSelectionSystem::SetMode(SelectionSystem::EMode mode) {
    // Only change something if the mode has actually changed
    if (_mode != mode) {
        _mode = mode;
        pivotChanged();
    }
}

// returns the currently active mode
SelectionSystem::EMode RadiantSelectionSystem::Mode() const {
    return _mode;
}

// Set the current component mode to <mode>
void RadiantSelectionSystem::SetComponentMode(EComponentMode mode) {
    _componentMode = mode;
}

// returns the current component mode
SelectionSystem::EComponentMode RadiantSelectionSystem::ComponentMode() const {
    return _componentMode;
}

std::size_t RadiantSelectionSystem::registerManipulator(const ManipulatorPtr& manipulator)
{
	std::size_t newId = 1;

	while (_manipulators.find(newId) != _manipulators.end())
	{
		++newId;

		if (newId == std::numeric_limits<std::size_t>::max())
		{
			throw std::runtime_error("Out of manipulator IDs");
		}
	}

	_manipulators.insert(std::make_pair(newId, manipulator));

	manipulator->setId(newId);

	if (!_activeManipulator)
	{
		_activeManipulator = manipulator;
	}

	return newId;
}

void RadiantSelectionSystem::unregisterManipulator(const ManipulatorPtr& manipulator)
{
	for (Manipulators::const_iterator i = _manipulators.begin(); i != _manipulators.end(); ++i)
	{
		if (i->second == manipulator)
		{
			i->second->setId(0);
			_manipulators.erase(i);
			return;
		}
	}
}

Manipulator::Type RadiantSelectionSystem::getActiveManipulatorType()
{
	return _activeManipulator->getType();
}

const ManipulatorPtr& RadiantSelectionSystem::getActiveManipulator()
{
	return _activeManipulator;
}

void RadiantSelectionSystem::setActiveManipulator(std::size_t manipulatorId)
{
	Manipulators::const_iterator found = _manipulators.find(manipulatorId);

	if (found == _manipulators.end())
	{
		rError() << "Cannot activate non-existent manipulator ID " << manipulatorId << std::endl;
		return;
	}

	_activeManipulator = found->second;

	// Release the user lock when switching manipulators
	_pivot.setUserLocked(false);

	pivotChanged();
}

void RadiantSelectionSystem::setActiveManipulator(Manipulator::Type manipulatorType)
{
	for (const Manipulators::value_type& pair : _manipulators)
	{
		if (pair.second->getType() == manipulatorType)
		{
			_activeManipulator = pair.second;

			// Release the user lock when switching manipulators
			_pivot.setUserLocked(false);

			pivotChanged();
			return;
		}
	}

	rError() << "Cannot activate non-existent manipulator by type " << manipulatorType << std::endl;
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

    // Schedule an idle callback
    requestIdleCallback();

    _requestWorkZoneRecalculation = true;
    _requestSceneGraphChange = true;

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

    // Schedule an idle callback
    requestIdleCallback();

    _requestWorkZoneRecalculation = true;
    _requestSceneGraphChange = true;

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
				componentSelectionTestable->setSelectedComponents(selected, SelectionSystem::eVertex);
				componentSelectionTestable->setSelectedComponents(selected, SelectionSystem::eEdge);
				componentSelectionTestable->setSelectedComponents(selected, SelectionSystem::eFace);
			}

			return true;
		});
	}

	_activeManipulator->setSelected(selected);
}

// Traverse the current selection and visit them with the given visitor class
void RadiantSelectionSystem::foreachSelected(const Visitor& visitor)
{
    for (SelectionListType::const_iterator i = _selection.begin();
         i != _selection.end();
         /* in-loop increment */)
    {
        visitor.visit((i++)->first);
    }
}

// Traverse the current selection components and visit them with the given visitor class
void RadiantSelectionSystem::foreachSelectedComponent(const Visitor& visitor)
{
    for (SelectionListType::const_iterator i = _componentSelection.begin();
         i != _componentSelection.end();
         /* in-loop increment */)
    {
        visitor.visit((i++)->first);
    }
}

void RadiantSelectionSystem::foreachSelected(const std::function<void(const scene::INodePtr&)>& functor)
{
	for (SelectionListType::const_iterator i = _selection.begin();
         i != _selection.end();
         /* in-loop increment */)
    {
        functor((i++)->first);
    }
}

void RadiantSelectionSystem::foreachSelectedComponent(const std::function<void(const scene::INodePtr&)>& functor)
{
	for (SelectionListType::const_iterator i = _componentSelection.begin();
         i != _componentSelection.end();
         /* in-loop increment */)
    {
        functor((i++)->first);
    }
}

void RadiantSelectionSystem::foreachBrush(const std::function<void(Brush&)>& functor)
{
	BrushSelectionWalker walker(functor);

	for (SelectionListType::const_iterator i = _selection.begin();
         i != _selection.end();
         /* in-loop increment */)
    {
		walker.visit((i++)->first); // Handles group nodes recursively
    }
}

void RadiantSelectionSystem::foreachFace(const std::function<void(Face&)>& functor)
{
	FaceSelectionWalker walker(functor);

	for (SelectionListType::const_iterator i = _selection.begin();
         i != _selection.end();
         /* in-loop increment */)
    {
		walker.visit((i++)->first); // Handles group nodes recursively
    }

	// Handle the component selection too
	algorithm::forEachSelectedFaceComponent(functor);
}

void RadiantSelectionSystem::foreachPatch(const std::function<void(Patch&)>& functor)
{
	PatchSelectionWalker walker(functor);

	for (SelectionListType::const_iterator i = _selection.begin();
         i != _selection.end();
         /* in-loop increment */)
    {
		walker.visit((i++)->first); // Handles group nodes recursively
    }
}

// Hub function for "deselect all", this passes the deselect call to the according functions
void RadiantSelectionSystem::deselectAll() {
    if (Mode() == eComponent) {
        setSelectedAllComponents(false);
    }
    else {
        setSelectedAll(false);
    }
}

/* greebo: This gets called by SelectObserver if the user just clicks into the scene (without dragging)
 * It checks for any possible targets (in the "line of click") and takes the actions according
 * to the modifiers that are held down (Alt-Shift, etc.)
 */
void RadiantSelectionSystem::SelectPoint(const render::View& view,
                                         const Vector2& device_point,
                                         const Vector2& device_epsilon,
                                         SelectionSystem::EModifier modifier,
                                         bool face)
{
    ASSERT_MESSAGE(fabs(device_point[0]) <= 1.0f && fabs(device_point[1]) <= 1.0f, "point-selection error");
    // If the user is holding the replace modifiers (default: Alt-Shift), deselect the current selection
    if (modifier == SelectionSystem::eReplace) {
        if (face) {
            setSelectedAllComponents(false);
        }
        else {
            deselectAll();
        }
    }

    {
        render::View scissored(view);
        // Construct a selection test according to a small box with 2*epsilon edge length
        ConstructSelectionTest(scissored, Rectangle::ConstructFromPoint(device_point, device_epsilon));

        // Create a new SelectionPool instance and fill it with possible candidates
        SelectionVolume volume(scissored);
        // The possible candidates are stored in the SelectablesSet
        SelectablesList candidates;

        if (face)
        {
            SelectionPool selector;

            ComponentSelector selectionTester(selector, volume, eFace);
            GlobalSceneGraph().foreachVisibleNodeInVolume(scissored, selectionTester);

            // Load them all into the vector
            for (SelectionPool::const_iterator i = selector.begin(); i != selector.end(); ++i)
            {
                candidates.push_back(i->second);
            }
        }
        else {
            testSelectScene(candidates, volume, scissored, Mode(), ComponentMode());
        }

        // Was the selection test successful (have we found anything to select)?
		performPointSelection(candidates, modifier);
    }
}

namespace algorithm
{

// If the selectable is a GroupSelectable, the respective callback is used
inline void setSelectionStatus(ISelectable* selectable, bool selected)
{
	IGroupSelectable* groupSelectable = dynamic_cast<IGroupSelectable*>(selectable);

	if (groupSelectable)
	{
		groupSelectable->setSelected(selected, true); // propagate selection state to group peers
	}
	else
	{
		selectable->setSelected(selected);
	}
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
			algorithm::setSelectionStatus(best, !best->isSelected());
		}
		break;

		// greebo: eReplace mode gets active as soon as the user holds the replace modifiers down
		// and clicks (by default: Alt-Shift). eReplace is only active during the first click
		// afterwards we are in cycle mode.
		// if cycle mode not enabled, enable it
		case eReplace:
		{
			// select closest (=first in the list)
			algorithm::setSelectionStatus(candidates.front(), true);
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
					algorithm::setSelectionStatus(*i, false);
					//(*i)->setSelected(false);

					// check if there is a "next" item in the list, if not: select the first item
					++i;

					if (i != candidates.end()) 
					{
						algorithm::setSelectionStatus(*i, true);
						//(*i)->setSelected(true);
					}
					else
					{
						algorithm::setSelectionStatus(candidates.front(), true);
						//candidates.front()->setSelected(true);
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

/* greebo: This gets called by the SelectObserver if the user drags a box and holds down
 * any of the selection modifiers. Possible selection candidates are determined and selected/deselected
 */
void RadiantSelectionSystem::SelectArea(const render::View& view,
                                        const Vector2& device_point,
                                        const Vector2& device_delta,
                                        SelectionSystem::EModifier modifier, bool face)
{
    // If we are in replace mode, deselect all the components or previous selections
    if (modifier == SelectionSystem::eReplace) {
        if (face) {
            setSelectedAllComponents(false);
        }
        else {
            deselectAll();
        }
    }

    {
        // Construct the selection test according to the area the user covered with his drag
        render::View scissored(view);
        ConstructSelectionTest(scissored, Rectangle::ConstructFromArea(device_point, device_delta));

        SelectionVolume volume(scissored);
        // The posssible candidates go here
        SelectionPool pool;

        SelectablesList candidates;

        if (face)
        {
            ComponentSelector selectionTester(pool, volume, eFace);
            GlobalSceneGraph().foreachVisibleNodeInVolume(scissored, selectionTester);

            // Load them all into the vector
            for (SelectionPool::const_iterator i = pool.begin(); i != pool.end(); ++i)
            {
                candidates.push_back(i->second);
            }
        }
        else {
            testSelectScene(candidates, volume, scissored, Mode(), ComponentMode());
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

		for (const SelectablesMap::value_type& state : selectableStates)
		{
			algorithm::setSelectionStatus(state.first, state.second);
		}
    }
}

void RadiantSelectionSystem::onManipulationStart()
{
	// Save the pivot state now that the transformation is starting
	_pivot.beginOperation();
}

void RadiantSelectionSystem::onManipulationChanged()
{
	_requestWorkZoneRecalculation = true;
	_requestSceneGraphChange = false;

	GlobalSceneGraph().sceneChanged();

	requestIdleCallback();
}

void RadiantSelectionSystem::onManipulationEnd()
{
	_pivot.endOperation();

	// The selection bounds have possibly changed, request an idle callback
	_requestWorkZoneRecalculation = true;
	_requestSceneGraphChange = true;

	requestIdleCallback();
}

void RadiantSelectionSystem::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const 
{
    renderSolid(collector, volume);
}

// Lets the recalculatePivot2World() method do the work and returns the result that is stored in the member variable
const Matrix4& RadiantSelectionSystem::getPivot2World()
{
    return _pivot.getMatrix4();
}

void RadiantSelectionSystem::captureShaders()
{
    TranslateManipulator::_stateWire = GlobalRenderSystem().capture("$WIRE_OVERLAY");
    TranslateManipulator::_stateFill = GlobalRenderSystem().capture("$FLATSHADE_OVERLAY");
    RotateManipulator::_stateOuter = GlobalRenderSystem().capture("$WIRE_OVERLAY");
	RotateManipulator::_pivotPointShader = GlobalRenderSystem().capture("$POINT");
	ModelScaleManipulator::_lineShader = GlobalRenderSystem().capture("$WIRE_OVERLAY");
	ModelScaleManipulator::_pointShader = GlobalRenderSystem().capture("$POINT");
}

void RadiantSelectionSystem::releaseShaders()
{
    TranslateManipulator::_stateWire.reset();
    TranslateManipulator::_stateFill.reset();
	RotateManipulator::_stateOuter.reset();
	RotateManipulator::_pivotPointShader.reset();
}

const WorkZone& RadiantSelectionSystem::getWorkZone()
{
    // Flush any pending idle callbacks, we need the workzone now
    flushIdleCallback();

    return _workZone;
}

/* greebo: Renders the currently active manipulator by setting the render state and
 * calling the manipulator's render method
 */
void RadiantSelectionSystem::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
    if (!nothingSelected())
	{
        collector.setHighlightFlag(RenderableCollector::Highlight::Faces, false);
        collector.setHighlightFlag(RenderableCollector::Highlight::Primitives, false);

		_activeManipulator->render(collector, volume);
    }
}

void RadiantSelectionSystem::onSceneBoundsChanged()
{
    // The bounds of the scenegraph have (possibly) changed
    pivotChanged();

    _requestWorkZoneRecalculation = true;
    requestIdleCallback();
}

// RegisterableModule implementation
const std::string& RadiantSelectionSystem::getName() const {
    static std::string _name(MODULE_SELECTIONSYSTEM);
    return _name;
}

const StringSet& RadiantSelectionSystem::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
	{
        _dependencies.insert(MODULE_RENDERSYSTEM);
        _dependencies.insert(MODULE_EVENTMANAGER);
        _dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_RADIANT);
        _dependencies.insert(MODULE_GRID);
        _dependencies.insert(MODULE_SCENEGRAPH);
        _dependencies.insert(MODULE_MOUSETOOLMANAGER);
		_dependencies.insert(MODULE_MAP);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
    }

    return _dependencies;
}

void RadiantSelectionSystem::initialiseModule(const ApplicationContext& ctx) 
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

	captureShaders();

	_pivot.initialise();

	// Add manipulators
	registerManipulator(std::make_shared<DragManipulator>(_pivot));
	registerManipulator(std::make_shared<ClipManipulator>());
	registerManipulator(std::make_shared<TranslateManipulator>(_pivot, 2, 64));
	registerManipulator(std::make_shared<ScaleManipulator>(_pivot, 0, 64));
	registerManipulator(std::make_shared<RotateManipulator>(_pivot, 8, 64));
	registerManipulator(std::make_shared<ModelScaleManipulator>(_pivot));

	_defaultManipulatorType = Manipulator::Drag;
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

	GlobalEventManager().addToggle("ToggleClipper", std::bind(&RadiantSelectionSystem::toggleManipulatorMode, this, Manipulator::Clip, std::placeholders::_1));
	GlobalEventManager().addToggle("MouseTranslate", std::bind(&RadiantSelectionSystem::toggleManipulatorMode, this, Manipulator::Translate, std::placeholders::_1));
	GlobalEventManager().addToggle("MouseRotate", std::bind(&RadiantSelectionSystem::toggleManipulatorMode, this, Manipulator::Rotate, std::placeholders::_1));
	GlobalEventManager().addToggle("MouseDrag", std::bind(&RadiantSelectionSystem::toggleManipulatorMode, this, Manipulator::Drag, std::placeholders::_1));
	GlobalEventManager().addToggle("ToggleModelScaleManipulator", std::bind(&RadiantSelectionSystem::toggleManipulatorMode, this, Manipulator::ModelScale, std::placeholders::_1));
	GlobalEventManager().setToggled("MouseDrag", true);

	GlobalEventManager().addToggle("DragVertices", std::bind(&RadiantSelectionSystem::toggleComponentMode, this, eVertex, std::placeholders::_1));
	GlobalEventManager().addToggle("DragEdges", std::bind(&RadiantSelectionSystem::toggleComponentMode, this, eEdge, std::placeholders::_1));
	GlobalEventManager().addToggle("DragFaces", std::bind(&RadiantSelectionSystem::toggleComponentMode, this, eFace, std::placeholders::_1));
	GlobalEventManager().addToggle("DragEntities", std::bind(&RadiantSelectionSystem::toggleEntityMode, this, std::placeholders::_1));
	GlobalEventManager().addToggle("SelectionModeGroupPart", std::bind(&RadiantSelectionSystem::toggleGroupPartMode, this, std::placeholders::_1));

	GlobalEventManager().setToggled("DragVertices", false);
	GlobalEventManager().setToggled("DragEdges", false);
	GlobalEventManager().setToggled("DragFaces", false);
	GlobalEventManager().setToggled("DragEntities", false);
	GlobalEventManager().setToggled("SelectionModeGroupPart", false);

	GlobalCommandSystem().addCommand("UnSelectSelection", std::bind(&RadiantSelectionSystem::deselectCmd, this, std::placeholders::_1));
	GlobalEventManager().addCommand("UnSelectSelection", "UnSelectSelection");

	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Selection"));

	page.appendCheckBox(_("Ignore light volume bounds when calculating default rotation pivot location"), 
		ManipulationPivot::RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES);

    // Connect the bounds changed caller
    GlobalSceneGraph().signal_boundsChanged().connect(
        sigc::mem_fun(this, &RadiantSelectionSystem::onSceneBoundsChanged)
    );

    GlobalRenderSystem().attachRenderable(*this);

    // Orthoview: manipulate and all the non-face selection tools
    ui::IMouseToolGroup& orthoGroup = GlobalMouseToolManager().getGroup(ui::IMouseToolGroup::Type::OrthoView);

    orthoGroup.registerMouseTool(std::make_shared<ui::ManipulateMouseTool>(*this));
    orthoGroup.registerMouseTool(std::make_shared<ui::DragSelectionMouseTool>());
    orthoGroup.registerMouseTool(std::make_shared<ui::CycleSelectionMouseTool>());

    // Camera: manipulation plus all selection tools, including the face-only tools
    ui::IMouseToolGroup& camGroup = GlobalMouseToolManager().getGroup(ui::IMouseToolGroup::Type::CameraView);

    camGroup.registerMouseTool(std::make_shared<ui::ManipulateMouseTool>(*this));
    camGroup.registerMouseTool(std::make_shared<ui::DragSelectionMouseTool>());
    camGroup.registerMouseTool(std::make_shared<ui::DragSelectionMouseToolFaceOnly>());
    camGroup.registerMouseTool(std::make_shared<ui::CycleSelectionMouseTool>());
    camGroup.registerMouseTool(std::make_shared<ui::CycleSelectionMouseToolFaceOnly>());

	GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &RadiantSelectionSystem::onMapEvent));
}

void RadiantSelectionSystem::shutdownModule() 
{
    // greebo: Unselect everything so that no references to scene::Nodes
    // are kept after shutdown, causing destruction issues.
    setSelectedAll(false);
    setSelectedAllComponents(false);

	// In pathological cases this list might contain remnants, clear it
	_selection.clear();

	_activeManipulator.reset();
	_manipulators.clear();

    GlobalRenderSystem().detachRenderable(*this);

    releaseShaders();
}

void RadiantSelectionSystem::checkComponentModeSelectionMode(const ISelectable& selectable)
{
	// This seems to be a fail-safe method, to detect situations where component mode is still
	// active without any primitive selected - in which case the method exits component mode.
	if (Mode() == eComponent && countSelected() == 0)
	{
		activateDefaultMode();
		onComponentModeChanged();
	}
}

void RadiantSelectionSystem::onIdle()
{
    // System is idle, check for pending tasks

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
                _workZone.max = Vector3(64,64,64);
                _workZone.min = Vector3(-64,-64,-64);
            }

            _workZone.bounds = bounds;
        }
    }

    // Check if we should notify the scenegraph
    if (_requestSceneGraphChange)
    {
        _requestSceneGraphChange = false;

        GlobalSceneGraph().sceneChanged();
    }
}

std::size_t RadiantSelectionSystem::getManipulatorIdForType(Manipulator::Type type)
{
	for (const Manipulators::value_type& pair : _manipulators)
	{
		if (pair.second->getType() == _defaultManipulatorType)
		{
			return pair.first;
		}
	}

	return 0;
}

void RadiantSelectionSystem::toggleManipulatorModeById(std::size_t manipId, bool newState)
{
	std::size_t defaultManipId = getManipulatorIdForType(_defaultManipulatorType);

	if (defaultManipId == 0)
	{
		return;
	}

	// Switch back to the default mode if we're already in <mode>
	if (_activeManipulator->getId() == manipId && defaultManipId != manipId)
	{
		toggleManipulatorModeById(defaultManipId, true);
	}
	else // we're not in <mode> yet
	{
		std::size_t clipperId = getManipulatorIdForType(Manipulator::Clip);

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

void RadiantSelectionSystem::toggleManipulatorMode(Manipulator::Type type, bool newState)
{
	// Switch back to the default mode if we're already in <mode>
	if (_activeManipulator->getType() == type && _defaultManipulatorType != type)
	{
		toggleManipulatorMode(_defaultManipulatorType, true);
	}
	else // we're not in <mode> yet
	{
		if (type == Manipulator::Clip)
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
	SetMode(ePrimitive);
	SetComponentMode(eDefault);

	SceneChangeNotify();
}

void RadiantSelectionSystem::toggleComponentMode(EComponentMode mode, bool newState)
{
	if (Mode() == eComponent && ComponentMode() == mode)
	{
		// De-select all the selected components before switching back
		setSelectedAllComponents(false);
		activateDefaultMode();
	}
	else if (countSelected() != 0)
	{
		if (!_activeManipulator->supportsComponentManipulation())
		{
			toggleManipulatorMode(_defaultManipulatorType, true);
		}

		SetMode(eComponent);
		SetComponentMode(mode);
	}

	onComponentModeChanged();
}

void RadiantSelectionSystem::toggleEntityMode(bool newState)
{
	if (Mode() == eEntity)
	{
		activateDefaultMode();
	}
	else 
	{
		SetMode(eEntity);
		SetComponentMode(eDefault);
	}

	onManipulatorModeChanged();
	onComponentModeChanged();
}

void RadiantSelectionSystem::toggleGroupPartMode(bool newState)
{
	if (Mode() == eGroupPart)
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

		SetMode(eGroupPart);
		SetComponentMode(eDefault);
	}

	onManipulatorModeChanged();
	onComponentModeChanged();
}

void RadiantSelectionSystem::onManipulatorModeChanged()
{
	GlobalEventManager().setToggled("ToggleClipper", GlobalClipper().clipMode());
	
	GlobalEventManager().setToggled("MouseTranslate", _activeManipulator->getType() == Manipulator::Translate);
	GlobalEventManager().setToggled("MouseRotate", _activeManipulator->getType() == Manipulator::Rotate);
	GlobalEventManager().setToggled("MouseDrag", _activeManipulator->getType() == Manipulator::Drag);
	GlobalEventManager().setToggled("ToggleModelScaleManipulator", _activeManipulator->getType() == Manipulator::ModelScale);

	SceneChangeNotify();
}

void RadiantSelectionSystem::onComponentModeChanged()
{
	GlobalEventManager().setToggled("DragVertices", Mode() == eComponent && ComponentMode() == eVertex);
	GlobalEventManager().setToggled("DragEdges", Mode() == eComponent && ComponentMode() == eEdge);
	GlobalEventManager().setToggled("DragFaces", Mode() == eComponent && ComponentMode() == eFace);
	GlobalEventManager().setToggled("DragEntities", Mode() == eEntity && ComponentMode() == eDefault);
	GlobalEventManager().setToggled("SelectionModeGroupPart", Mode() == eGroupPart && ComponentMode() == eDefault);

	SceneChangeNotify();
}

// called when the escape key is used (either on the main window or on an inspector)
void RadiantSelectionSystem::deselectCmd(const cmd::ArgumentList& args)
{
	if (Mode() == eComponent)
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
			setSelectedAll(false);
		}
	}
}

void RadiantSelectionSystem::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloading || ev == IMap::MapLoading)
	{
		setSelectedAll(false);
		setSelectedAllComponents(false);
	}
}

// Define the static SelectionSystem module
module::StaticModule<RadiantSelectionSystem> radiantSelectionSystemModule;

}
