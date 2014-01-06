#include "RadiantSelectionSystem.h"

#include "gdk/gdktypes.h"

#include "iundo.h"
#include "igrid.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "editable.h"
#include "Selectors.h"
#include "SelectionTest.h"
#include "SceneWalkers.h"
#include "patch/PatchSceneWalk.h"
#include "xyview/GlobalXYWnd.h"
#include "modulesystem/StaticModule.h"
#include "registry/registry.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/General.h"

#include <boost/bind.hpp>

// Initialise the shader pointer
ShaderPtr RadiantSelectionSystem::_state;

    namespace {
        const std::string RKEY_ROTATION_PIVOT = "user/ui/rotationPivotIsOrigin";
    }

// ------------ Helper Functions --------------------------------------------

inline void matrix4_assign_rotation(Matrix4& matrix, const Matrix4& other) {
    matrix[0] = other[0];
    matrix[1] = other[1];
    matrix[2] = other[2];
    matrix[4] = other[4];
    matrix[5] = other[5];
    matrix[6] = other[6];
    matrix[8] = other[8];
    matrix[9] = other[9];
    matrix[10] = other[10];
}

void matrix4_assign_rotation_for_pivot(Matrix4& matrix, const scene::INodePtr& node) {
    EditablePtr editable = Node_getEditable(node);
    // If the instance is editable, take the localpivot point into account, otherwise just apply the rotation
    if (editable != 0) {
        matrix4_assign_rotation(matrix, node->localToWorld().getMultipliedBy(editable->getLocalPivot()));
    }
    else {
        matrix4_assign_rotation(matrix, node->localToWorld());
    }
}

// --------- RadiantSelectionSystem Implementation ------------------------------------------

RadiantSelectionSystem::RadiantSelectionSystem() :
    _requestSceneGraphChange(false),
    _requestWorkZoneRecalculation(true),
	_defaultManipulatorMode(eDrag),
	_manipulatorMode(_defaultManipulatorMode),
	_currentManipulatorModeSupportsComponentEditing(false),
    _undoBegun(false),
    _mode(ePrimitive),
    _componentMode(eDefault),
    _countPrimitive(0),
    _countComponent(0),
    _translateManipulator(*this, 2, 64),    // initialise the Manipulators with a pointer to self
    _rotateManipulator(*this, 8, 64),
    _scaleManipulator(*this, 0, 64),
    _pivotChanged(false),
    _pivotMoving(false)
{}

const SelectionInfo& RadiantSelectionSystem::getSelectionInfo() {
    return _selectionInfo;
}

void RadiantSelectionSystem::addObserver(Observer* observer) {
    if (observer != NULL) {
        // Add the passed observer to the list
        _observers.push_back(observer);
    }
}

void RadiantSelectionSystem::removeObserver(Observer* observer) {
    // Cycle through the list of observers and call the moved method
    for (ObserverList::iterator i = _observers.begin(); i != _observers.end(); ++i) {
        Observer* registered = *i;

        if (registered == observer) {
            _observers.erase(i);
            return; // Don't continue the loop, the iterator is obsolete
        }
    }
}

void RadiantSelectionSystem::notifyObservers(const scene::INodePtr& node, bool isComponent) {

    // Cycle through the list of observers and call the moved method
    for (ObserverList::iterator i = _observers.begin(); i != _observers.end(); ++i) {
        Observer* observer = *i;

        if (observer != NULL) {
            observer->selectionChanged(node, isComponent);
        }
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

            for (SelectionPool::iterator i = selector.begin(); i != selector.end(); ++i)
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
            for (SelectionPool::iterator i = selector.begin(); i != selector.end(); ++i) {
                targetList.push_back(i->second);
            }

            // Add the secondary crop to the vector (if it has any entries)
            for (SelectionPool::iterator i = sel2.begin(); i != sel2.end(); ++i) {
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
            for (SelectionPool::iterator i = selector.begin(); i != selector.end(); ++i)
            {
                targetList.push_back(i->second);
            }
        }
        break;

        case eComponent:
        {
            ComponentSelector selectionTester(selector, test, componentMode);
            foreachSelected(selectionTester);

            for (SelectionPool::iterator i = selector.begin(); i != selector.end(); ++i)
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

void RadiantSelectionSystem::pivotChanged() const  
{
    _pivotChanged = true;
    SceneChangeNotify();
}

void RadiantSelectionSystem::pivotChangedSelection(const Selectable& selectable) {
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

// Set the current manipulator mode to <mode>
void RadiantSelectionSystem::SetManipulatorMode(EManipulatorMode mode) {
    _manipulatorMode = mode;
    switch(_manipulatorMode) {
        case eTranslate: _manipulator = &_translateManipulator; break;
        case eRotate: _manipulator = &_rotateManipulator; break;
        case eScale: _manipulator = &_scaleManipulator; break;
        case eDrag: _manipulator = &_dragManipulator; break;
        case eClip: _manipulator = &_clipManipulator; break;
    }
    pivotChanged();
}

// return the current manipulator mode
SelectionSystem::EManipulatorMode RadiantSelectionSystem::ManipulatorMode() const {
    return _manipulatorMode;
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
void RadiantSelectionSystem::onSelectedChanged(const scene::INodePtr& node, const Selectable& selectable) {

    // Cache the selection state
    bool isSelected = selectable.isSelected();
    int delta = isSelected ? +1 : -1;

    _countPrimitive += delta;
    
    _selectionInfo.totalCount += delta;

    if (Node_getPatch(node) != NULL) {
        _selectionInfo.patchCount += delta;
    }
    else if (Node_getBrush(node) != NULL) {
        _selectionInfo.brushCount += delta;
    }
    else {
        _selectionInfo.entityCount += delta;
    }

    // If the selectable is selected, add it to the local selection list, otherwise remove it
    if (isSelected) {
        _selection.append(node);
    }
    else {
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
}

// greebo: This should be called "onComponentSelectionChanged", as it is a similar function of the above one
// Updates the internal list of component nodes if the component selection gets changed
void RadiantSelectionSystem::onComponentSelection(const scene::INodePtr& node, const Selectable& selectable) {

    int delta = selectable.isSelected() ? +1 : -1;

    _countComponent += delta;
    _sigSelectionChanged(selectable);

    _selectionInfo.totalCount += delta;
    _selectionInfo.componentCount += delta;

    // If the instance got selected, add it to the list, otherwise remove it
    if (selectable.isSelected()) {
        _componentSelection.append(node);
    }
    else {
        _componentSelection.erase(node);
    }

    // Notify observers, TRUE => this is a component selection change
    notifyObservers(node, true);

    // Check if the number of selected components in the list matches the value of the selection counter
    ASSERT_MESSAGE(_componentSelection.size() == _countComponent, "component selection-tracking error");

    // Schedule an idle callback
    requestIdleCallback();

    _requestWorkZoneRecalculation = true;
    _requestSceneGraphChange = true;
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

    _manipulator->setSelected(selected);
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

    _manipulator->setSelected(selected);
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
	selection::algorithm::forEachSelectedFaceComponent(functor);
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

// Start a move, the current pivot point is saved as a start point
void RadiantSelectionSystem::startMove() {
    _pivot2worldStart = GetPivot2World();
}

/* greebo: This is called by the ManipulateObserver class on the mouseDown event. It checks, if a manipulator
 * can be selected where the mouse is pointing to.
 */
bool RadiantSelectionSystem::SelectManipulator(const render::View& view, const Vector2& device_point, const Vector2& device_epsilon)
{
    if (!nothingSelected() || (ManipulatorMode() == eDrag && Mode() == eComponent))
    {
        // Unselect any currently selected manipulators to be sure
        _manipulator->setSelected(false);

        // Test, if the current manipulator can be selected
        if (!nothingSelected() || (ManipulatorMode() == eDrag && Mode() == eComponent))
        {
            render::View scissored(view);
            ConstructSelectionTest(scissored, Rectangle::ConstructFromPoint(device_point, device_epsilon));

            // The manipulator class checks on its own, if any of its components can be selected
            _manipulator->testSelect(scissored, GetPivot2World());
        }

        // Save the pivot2world matrix
        startMove();

        // This is true, if a manipulator could be selected
        _pivotMoving = _manipulator->isSelected();

        // is a manipulator selected / the pivot moving?
        if (_pivotMoving) {
            Pivot2World pivot;
            pivot.update(GetPivot2World(), view.GetModelview(), view.GetProjection(), view.GetViewport());

            _manip2pivotStart = _pivot2worldStart.getFullInverse().getMultipliedBy(pivot._worldSpace);

            Matrix4 device2manip;
            ConstructDevice2Manip(device2manip, _pivot2worldStart, view.GetModelview(), view.GetProjection(), view.GetViewport());
            _manipulator->getActiveComponent()->Construct(device2manip, device_point[0], device_point[1]);

            _deviceStart = Vector2(device_point[0], device_point[1]);

            _undoBegun = false;
        }

        SceneChangeNotify();
    }

    return _pivotMoving;
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
            for (SelectionPool::iterator i = selector.begin(); i != selector.end(); ++i)
            {
                candidates.push_back(i->second);
            }
        }
        else {
            testSelectScene(candidates, volume, scissored, Mode(), ComponentMode());
        }

        // Was the selection test successful (have we found anything to select)?
        if (candidates.size() > 0) {
            // Yes, now determine how we should interpret the click
            switch (modifier) {
                // If we are in toggle mode (Shift-Left-Click by default), just toggle the
                // selection of the "topmost" item
                case SelectionSystem::eToggle: {
                    Selectable* best = *candidates.begin();
                    // toggle selection of the object with least depth (=first in the list)
                    best->setSelected(!best->isSelected());
                }
                break;
                // greebo: eReplace mode gets active as soon as the user holds the replace modifiers down
                // and clicks (by default: Alt-Shift). eReplace is only active during the first click
                // afterwards we are in cycle mode.
                // if cycle mode not enabled, enable it
                case SelectionSystem::eReplace: {
                    // select closest (=first in the list)
                    (*candidates.begin())->setSelected(true);
                }
                break;
                // select the next object in the list from the one already selected
                // greebo: eCycle is set if the user keeps holding the replace modifiers (Alt-Shift)
                // and does NOT move the mouse between the clicks, otherwise we fall back into eReplace mode
                // Note: The mode is set in SelectObserver::testSelect()
                case SelectionSystem::eCycle: {
                    // Cycle through the selection pool and activate the item right after the currently selected
                    SelectablesList::iterator i = candidates.begin();

                    while (i != candidates.end()) {
                        if ((*i)->isSelected()) {
                            // unselect the currently selected one
                            (*i)->setSelected(false);
                            // check if there is a "next" item in the list, if not: select the first item
                            ++i;
                            if (i != candidates.end()) {
                                (*i)->setSelected(true);
                            }
                            else {
                                (*candidates.begin())->setSelected(true);
                            }
                            break;
                        }
                        ++i;
                    } // while
                } // case
                break;
                default:
                break;
            } // switch
        }
    }
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
            for (SelectionPool::iterator i = pool.begin(); i != pool.end(); ++i)
            {
                candidates.push_back(i->second);
            }
        }
        else {
            testSelectScene(candidates, volume, scissored, Mode(), ComponentMode());
        }

        // Cycle through the selection pool and toggle the candidates, but only if we are in toggle mode
        for (SelectablesList::iterator i = candidates.begin(); i != candidates.end(); i++) {
            (*i)->setSelected(!(modifier == SelectionSystem::eToggle && (*i)->isSelected()));
        }
    }
}

// Applies the translation vector <translation> to the current selection
void RadiantSelectionSystem::translate(const Vector3& translation) {
    // Check if we have anything to do at all
    if (!nothingSelected()) {
        // Store the translation vector, so that the outputTranslation member method can access it
        _translation = translation;

        // Get the current pivot matrix and multiply it by the translation matrix defined by <translation>.
        _pivot2world = _pivot2worldStart;
        _pivot2world.translateBy(translation);

        // Call the according scene graph traversors and pass the translation vector
        if (Mode() == eComponent) {
            Scene_Translate_Component_Selected(GlobalSceneGraph(), _translation);
        }
        else {
            Scene_Translate_Selected(GlobalSceneGraph(), _translation);
        }

        // Update the scene so that the changes are made visible
        SceneChangeNotify();
    }
}

// Applies the rotation vector <rotation> to the current selection
void RadiantSelectionSystem::rotate(const Quaternion& rotation) {
    // Check if there is anything to do
    if (!nothingSelected()) {
        // Store the quaternion internally
        _rotation = rotation;

        // Perform the rotation according to the current mode
        if (Mode() == eComponent) {
            Scene_Rotate_Component_Selected(GlobalSceneGraph(), _rotation, _pivot2world.t().getVector3());

            matrix4_assign_rotation_for_pivot(_pivot2world, _componentSelection.ultimate());
        }
        else {
            Scene_Rotate_Selected(GlobalSceneGraph(), _rotation, _pivot2world.t().getVector3());

            matrix4_assign_rotation_for_pivot(_pivot2world, _selection.ultimate());
        }

        // Update the views
        SceneChangeNotify();
    }
}

// Applies the scaling vector <scaling> to the current selection, this is called by the according ManipulatorComponents
void RadiantSelectionSystem::scale(const Vector3& scaling) {
    // Check if anything is selected
    if (!nothingSelected()) {
        // Store the scaling vector internally
        _scale = scaling;

        // Pass the scale to the according traversor
        if (Mode() == eComponent) {
            Scene_Scale_Component_Selected(GlobalSceneGraph(), _scale, _pivot2world.t().getVector3());
        }
        else {
            Scene_Scale_Selected(GlobalSceneGraph(), _scale, _pivot2world.t().getVector3());
        }

        // Update the scene views
        SceneChangeNotify();
    }
}

// Dump the translation, rotation, scale to the output stream
void RadiantSelectionSystem::outputTranslation(std::ostream& ostream) {
    ostream << " -xyz " << _translation.x() << " " << _translation.y() << " " << _translation.z();
}

void RadiantSelectionSystem::outputRotation(std::ostream& ostream) {
    ostream << " -eulerXYZ " << _rotation.x() << " " << _rotation.y() << " " << _rotation.z();
}

void RadiantSelectionSystem::outputScale(std::ostream& ostream) {
    ostream << " -scale " << _scale.x() << " " << _scale.y() << " " << _scale.z();
}

// Shortcut call for an instantly applied rotation of the current selection
void RadiantSelectionSystem::rotateSelected(const Quaternion& rotation) {
    // Apply the transformation and freeze the changes
    startMove();
    rotate(rotation);
    freezeTransforms();
}

// Shortcut call for an instantly applied translation of the current selection
void RadiantSelectionSystem::translateSelected(const Vector3& translation) {
    // Apply the transformation and freeze the changes
    startMove();
    translate(translation);
    freezeTransforms();
}

// Shortcut call for an instantly applied scaling of the current selection
void RadiantSelectionSystem::scaleSelected(const Vector3& scaling) {
    // Apply the transformation and freeze the changes
    startMove();
    scale(scaling);
    freezeTransforms();
}

/* greebo: This "moves" the current selection. It calculates the device manipulation matrix
 * and passes it to the currently active Manipulator.
 */
void RadiantSelectionSystem::MoveSelected(const render::View& view, const Vector2& devicePoint)
{
    // Check, if the active manipulator is selected in the first place
    if (_manipulator->isSelected()) {
        // Initalise the undo system, if not yet done
        if (!_undoBegun) {
            _undoBegun = true;
            GlobalUndoSystem().start();
        }

        Matrix4 device2manip;
        ConstructDevice2Manip(device2manip, _pivot2worldStart, view.GetModelview(), view.GetProjection(), view.GetViewport());

        Vector2 constrainedDevicePoint(devicePoint);

        // Constrain the movement to the axes, if the modifier is held
        if ((GlobalEventManager().getModifierState() & GDK_SHIFT_MASK) != 0)
        {
            // Get the movement delta relative to the start point
            Vector2 delta = devicePoint - _deviceStart;

            // Set the "minor" value of the movement to zero
            if (fabs(delta[0]) > fabs(delta[1])) {
                // X axis is major, reset the y-value to the start
                delta[1] = 0;
            }
            else {
                // Y axis is major, reset the x-value to the start
                delta[0] = 0;
            }

            // Add the modified delta to the start point, constrained to one axis
            constrainedDevicePoint = _deviceStart + delta;
        }

        // Get the manipulatable from the currently active manipulator (done by selection test)
        // and call the Transform method (can be anything)
        _manipulator->getActiveComponent()->Transform(_manip2pivotStart, device2manip, constrainedDevicePoint[0], constrainedDevicePoint[1]);

        _requestWorkZoneRecalculation = true;
        _requestSceneGraphChange = true;

        requestIdleCallback();
    }
}

// greebo: This just passes the call on to renderSolid, the manipulators are wireframes anyway
void RadiantSelectionSystem::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const {
    renderSolid(collector, volume);
}

// Lets the ConstructPivot() method do the work and returns the result that is stored in the member variable
const Matrix4& RadiantSelectionSystem::GetPivot2World() const
{
    // Questionable const design - almost everything needs to be declared const here...
    const_cast<RadiantSelectionSystem*>(this)->ConstructPivot();

    return _pivot2world;
}

void RadiantSelectionSystem::constructStatic() {
    _state = GlobalRenderSystem().capture("$POINT");
    TranslateManipulator::_stateWire = GlobalRenderSystem().capture("$WIRE_OVERLAY");
    TranslateManipulator::_stateFill = GlobalRenderSystem().capture("$FLATSHADE_OVERLAY");
    RotateManipulator::_stateOuter = GlobalRenderSystem().capture("$WIRE_OVERLAY");
}

void RadiantSelectionSystem::destroyStatic() {
    _state = ShaderPtr();
    TranslateManipulator::_stateWire = ShaderPtr();
    TranslateManipulator::_stateFill = ShaderPtr();
    RotateManipulator::_stateOuter = ShaderPtr();
}

void RadiantSelectionSystem::cancelMove() {

    // Unselect any currently selected manipulators to be sure
    _manipulator->setSelected(false);

    // Tell all the scene objects to revert their transformations
	foreachSelected([] (const scene::INodePtr& node)
	{
		ITransformablePtr transform = Node_getTransformable(node);

		if (transform != NULL)
		{
			transform->revertTransform();
		}
	});

    _pivotMoving = false;
    pivotChanged();

    // greebo: Deselect all faces if we are in brush and drag mode
    if (Mode() == ePrimitive && ManipulatorMode() == eDrag)
    {
        SelectAllComponentWalker faceSelector(false, SelectionSystem::eFace);
        GlobalSceneGraph().root()->traverse(faceSelector);
    }

    if (_undoBegun) {
        // Cancel the undo operation, if one has been begun
        GlobalUndoSystem().cancel();
        _undoBegun = false;
    }

    // Update the views
    SceneChangeNotify();
}

// This actually applies the transformation to the objects
void RadiantSelectionSystem::freezeTransforms()
{
	GlobalSceneGraph().foreachNode(scene::freezeTransformableNode);
    
    // The selection bounds have possibly changed, request an idle callback
    _requestWorkZoneRecalculation = true;
    _requestSceneGraphChange = true;

    requestIdleCallback();
}

// End the move, this freezes the current transforms
void RadiantSelectionSystem::endMove() {
    freezeTransforms();

    // greebo: Deselect all faces if we are in brush and drag mode
    if ((Mode() == ePrimitive || Mode() == eGroupPart) &&
        ManipulatorMode() == eDrag)
    {
        SelectAllComponentWalker faceSelector(false, SelectionSystem::eFace);
        GlobalSceneGraph().root()->traverse(faceSelector);
    }

    // Remove all degenerated brushes from the scene graph (should emit a warning)
    foreachSelected(RemoveDegenerateBrushWalker());

    _pivotMoving = false;
    pivotChanged();

    // Update the views
    SceneChangeNotify();

    // If we started an undoable operation, end it now and tell the console what happened
    if (_undoBegun)
    {
        std::ostringstream command;

        if (ManipulatorMode() == eTranslate) {
            command << "translateTool";
            outputTranslation(command);
        }
        else if (ManipulatorMode() == eRotate) {
            command << "rotateTool";
            outputRotation(command);
        }
        else if (ManipulatorMode() == eScale) {
            command << "scaleTool";
            outputScale(command);
        }
        else if (ManipulatorMode() == eDrag) {
            command << "dragTool";
        }

        _undoBegun = false;

        // Finish the undo move
        GlobalUndoSystem().finish(command.str());
    }
}

const selection::WorkZone& RadiantSelectionSystem::getWorkZone()
{
    // Flush any pending idle callbacks, we need the workzone now
    flushIdleCallback();

    return _workZone;
}

void RadiantSelectionSystem::keyChanged()
{
    if (!nothingSelected()) {
        pivotChanged();
        ConstructPivot();
    }
}

/* greebo: This calculates and constructs the pivot point of the selection.
 * It cycles through all selected objects and creates its AABB. The origin point of the AABB
 * is basically the pivot point. Pivot2World is therefore a translation from (0,0,0) to the calculated origin.
 *
 * The pivot point is also snapped to the grid.
 */
void RadiantSelectionSystem::ConstructPivot()
{
    if (!_pivotChanged || _pivotMoving)
        return;

    _pivotChanged = false;

    Vector3 objectPivot;

    if (!nothingSelected())
    {
        if (_selectionInfo.entityCount == 1 && _selectionInfo.totalCount == 1 &&
            registry::getValue<bool>(RKEY_ROTATION_PIVOT))
        {
            // Test, if a single entity is selected
            scene::INodePtr node = ultimateSelected();
            Entity* entity = Node_getEntity(node);

            if (entity != NULL)
            {
                objectPivot = string::convert<Vector3>(
                    entity->getKeyValue("origin")
                );
            }
        }
        else {
            // Create a local variable where the aabb information is stored
            AABB bounds;

            // Traverse through the selection and update the <bounds> variable
            if (Mode() == eComponent)
            {
                bounds = selection::algorithm::getCurrentComponentSelectionBounds();
            }
            else
			{
				bounds = selection::algorithm::getCurrentSelectionBounds();
            }

            // the <bounds> variable now contains the AABB of the selection, retrieve the origin
            objectPivot = bounds.origin;
        }

        // Snap the pivot point to the grid (greebo: disabled this (issue #231))
        //vector3_snap(objectPivot, GlobalGrid().getGridSize());

        // The pivot2world matrix is just a translation from the world origin (0,0,0) to the object pivot
        _pivot2world = Matrix4::getTranslation(objectPivot);

        // Only rotation and scaling need further calculations
        switch (_manipulatorMode) {
            case eTranslate:
                break;
            case eRotate:
                if (Mode() == eComponent) {
                    matrix4_assign_rotation_for_pivot(_pivot2world, _componentSelection.ultimate());
                }
                else {
                    matrix4_assign_rotation_for_pivot(_pivot2world, _selection.ultimate());
                }
                break;
            case eScale:
                if (Mode() == eComponent) {
                    matrix4_assign_rotation_for_pivot(_pivot2world, _componentSelection.ultimate());
                }
                else {
                    matrix4_assign_rotation_for_pivot(_pivot2world, _selection.ultimate());
                }
                break;
            default:
                break;
        } // switch
    }
}
/* greebo: Renders the currently active manipulator by setting the render state and
 * calling the manipulator's render method
 */
void RadiantSelectionSystem::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const {
    if (!nothingSelected()) {
        collector.highlightFaces(false);
        collector.highlightPrimitives(false);

        collector.SetState(_state, RenderableCollector::eWireframeOnly);
        collector.SetState(_state, RenderableCollector::eFullMaterials);

        _manipulator->render(collector, volume, GetPivot2World());
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

const StringSet& RadiantSelectionSystem::getDependencies() const {
    static StringSet _dependencies;

    if (_dependencies.empty()) {
        _dependencies.insert(MODULE_RENDERSYSTEM);
        _dependencies.insert(MODULE_EVENTMANAGER);
        _dependencies.insert(MODULE_XMLREGISTRY);
        _dependencies.insert(MODULE_GRID);
        _dependencies.insert(MODULE_SCENEGRAPH);
    }

    return _dependencies;
}

void RadiantSelectionSystem::initialiseModule(const ApplicationContext& ctx) 
{
    rMessage() << "RadiantSelectionSystem::initialiseModule called.\n";

    constructStatic();

    SetManipulatorMode(eDrag);
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

    GlobalRegistry().signalForKey(RKEY_ROTATION_PIVOT).connect(
        sigc::mem_fun(this, &RadiantSelectionSystem::keyChanged)
    );

    // Pass a reference to self to the global event manager
    GlobalEventManager().connectSelectionSystem(this);

	GlobalEventManager().addToggle("ToggleClipper", boost::bind(&RadiantSelectionSystem::toggleClipManipulatorMode, this, _1));
	GlobalEventManager().addToggle("MouseTranslate", boost::bind(&RadiantSelectionSystem::toggleTranslateManipulatorMode, this, _1));
	GlobalEventManager().addToggle("MouseRotate", boost::bind(&RadiantSelectionSystem::toggleRotateManipulatorMode, this, _1));
	GlobalEventManager().addToggle("MouseDrag", boost::bind(&RadiantSelectionSystem::toggleDragManipulatorMode, this, _1));
	GlobalEventManager().setToggled("MouseDrag", true);

	GlobalEventManager().addToggle("DragVertices", boost::bind(&RadiantSelectionSystem::toggleVertexComponentMode, this, _1));
	GlobalEventManager().addToggle("DragEdges", boost::bind(&RadiantSelectionSystem::toggleEdgeComponentMode, this, _1));
	GlobalEventManager().addToggle("DragFaces", boost::bind(&RadiantSelectionSystem::toggleFaceComponentMode, this, _1));
	GlobalEventManager().addToggle("DragEntities", boost::bind(&RadiantSelectionSystem::toggleEntityMode, this, _1));
	GlobalEventManager().addToggle("SelectionModeGroupPart", boost::bind(&RadiantSelectionSystem::toggleGroupPartMode, this, _1));

	GlobalEventManager().setToggled("DragVertices", false);
	GlobalEventManager().setToggled("DragEdges", false);
	GlobalEventManager().setToggled("DragFaces", false);
	GlobalEventManager().setToggled("DragEntities", false);
	GlobalEventManager().setToggled("SelectionModeGroupPart", false);

	GlobalCommandSystem().addCommand("UnSelectSelection", boost::bind(&RadiantSelectionSystem::deselectCmd, this, _1));
	GlobalEventManager().addCommand("UnSelectSelection", "UnSelectSelection");

    // Connect the bounds changed caller
    GlobalSceneGraph().signal_boundsChanged().connect(
        sigc::mem_fun(this, &RadiantSelectionSystem::onSceneBoundsChanged)
    );

    GlobalRenderSystem().attachRenderable(*this);
}

void RadiantSelectionSystem::shutdownModule() 
{
    // greebo: Unselect everything so that no references to scene::Nodes
    // are kept after shutdown, causing destruction issues.
    setSelectedAll(false);
    setSelectedAllComponents(false);

    GlobalRenderSystem().detachRenderable(*this);

    destroyStatic();
}

void RadiantSelectionSystem::checkComponentModeSelectionMode(const Selectable& selectable)
{
	// This seems to be a fail-safe method, to detect situations where component mode is still
	// active without any primitive selected - in which case the method exits component mode.
	if (Mode() == eComponent && countSelected() == 0)
	{
		activateDefaultMode();
		onComponentModeChanged();
	}
}

void RadiantSelectionSystem::onGtkIdle()
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
			AABB bounds = selection::algorithm::getCurrentSelectionBounds();

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

void RadiantSelectionSystem::toggleDefaultManipulatorMode(bool newState)
{
	switch (_defaultManipulatorMode)
	{
		case eTranslate: toggleTranslateManipulatorMode(true); break;
		case eRotate: toggleRotateManipulatorMode(true); break;
		case eScale: break;
		case eDrag: toggleDragManipulatorMode(true); break;
		case eClip: toggleClipManipulatorMode(true); break;
	};
}

void RadiantSelectionSystem::toggleManipulatorMode(EManipulatorMode mode, bool newState)
{
	// Switch back to the default mode if we're already in <mode>
	if (_manipulatorMode == mode && _defaultManipulatorMode != mode)
	{
		toggleDefaultManipulatorMode(true);
	}
	else // we're not in <mode> yet
	{
		_currentManipulatorModeSupportsComponentEditing = true;

		GlobalClipper().onClipMode(false);
		SetManipulatorMode(mode);
		
		onManipulatorModeChanged();
	}
}

void RadiantSelectionSystem::toggleDragManipulatorMode(bool newState)
{
	toggleManipulatorMode(eDrag, newState); // pass the call to the generic method
}

void RadiantSelectionSystem::toggleTranslateManipulatorMode(bool newState)
{
	toggleManipulatorMode(eTranslate, newState); // pass the call to the generic method
}

void RadiantSelectionSystem::toggleRotateManipulatorMode(bool newState)
{
	toggleManipulatorMode(eRotate, newState); // pass the call to the generic method
}

void RadiantSelectionSystem::toggleClipManipulatorMode(bool newState)
{
	if (_manipulatorMode == eClip && _defaultManipulatorMode != eClip)
	{
		toggleDefaultManipulatorMode(true);
	}
	else
	{
		_currentManipulatorModeSupportsComponentEditing = false;

		activateDefaultMode();
		GlobalClipper().onClipMode(true);
		SetManipulatorMode(eClip);

		onManipulatorModeChanged();
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
		if (!_currentManipulatorModeSupportsComponentEditing)
		{
			toggleDefaultManipulatorMode(true);
		}

		SetMode(eComponent);
		SetComponentMode(mode);
	}

	onComponentModeChanged();
}

void RadiantSelectionSystem::toggleVertexComponentMode(bool newState)
{
	toggleComponentMode(eVertex, newState); // pass the call to the generic method
}

void RadiantSelectionSystem::toggleFaceComponentMode(bool newState)
{
	toggleComponentMode(eFace, newState); // pass the call to the generic method
}

void RadiantSelectionSystem::toggleEdgeComponentMode(bool newState)
{
	toggleComponentMode(eEdge, newState); // pass the call to the generic method
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
		setSelectedAll(false);
		setSelectedAllComponents(false);

		SetMode(eGroupPart);
		SetComponentMode(eDefault);
	}

	onManipulatorModeChanged();
	onComponentModeChanged();
}

void RadiantSelectionSystem::onManipulatorModeChanged()
{
	GlobalEventManager().setToggled("ToggleClipper", GlobalClipper().clipMode());
	
	GlobalEventManager().setToggled("MouseTranslate", ManipulatorMode() == eTranslate);
	GlobalEventManager().setToggled("MouseRotate", ManipulatorMode() == eRotate);
	GlobalEventManager().setToggled("MouseDrag", ManipulatorMode() == eDrag);

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

// Define the static SelectionSystem module
module::StaticModule<RadiantSelectionSystem> radiantSelectionSystemModule;
