#include "RadiantSelectionSystem.h"

#include "iundo.h"
#include "igrid.h"
#include "ieventmanager.h"
#include "renderer.h"
#include "scenelib.h"
#include "editable.h"
#include "stream/stringstream.h"
#include "Selectors.h"
#include "SelectionTest.h"
#include "SceneWalkers.h"

// Initialise the shader pointer
Shader* RadiantSelectionSystem::_state = 0;

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

void matrix4_assign_rotation_for_pivot(Matrix4& matrix, scene::Instance& instance) {
	Editable* editable = Node_getEditable(instance.path().top());
	// If the instance is editable, take the localpivot point into account, otherwise just apply the rotation
	if (editable != 0) {
		matrix4_assign_rotation(matrix, matrix4_multiplied_by_matrix4(instance.localToWorld(), editable->getLocalPivot()));
	}
	else {
		matrix4_assign_rotation(matrix, instance.localToWorld());
	}
}

// --------- RadiantSelectionSystem Implementation ------------------------------------------

RadiantSelectionSystem::RadiantSelectionSystem() :
	_undoBegun(false),
	_mode(ePrimitive),
	_componentMode(eDefault),
	_countPrimitive(SelectionChangedCaller(*this)),
	_countComponent(SelectionChangedCaller(*this)),
	_translateManipulator(*this, 2, 64),	// initialise the Manipulators with a pointer to self 
	_rotateManipulator(*this, 8, 64),
	_scaleManipulator(*this, 0, 64),
	_pivotChanged(false),
	_pivotMoving(false)
{
	SetManipulatorMode(eTranslate);
	pivotChanged();
	addSelectionChangeCallback(PivotChangedSelectionCaller(*this));
	GlobalGrid().addGridChangeCallback(PivotChangedCaller(*this));
	
	// Pass a reference to self to the global event manager 
	GlobalEventManager().connectSelectionSystem(this);
}

void RadiantSelectionSystem::Scene_TestSelect(Selector& selector, SelectionTest& test, const View& view, SelectionSystem::EMode mode, SelectionSystem::EComponentMode componentMode) {
	switch(mode) {
		case eEntity: {
			Scene_forEachVisible(GlobalSceneGraph(), view, testselect_entity_visible(selector, test));
		}
		break;
		case ePrimitive:
			Scene_TestSelect_Primitive(selector, test, view);
		break;
		case eComponent:
			Scene_TestSelect_Component_Selected(selector, test, view, componentMode);
		break;
	} // switch
}

/* greebo: This is true if nothing is selected (either in component mode or in primitive mode)  
 */
bool RadiantSelectionSystem::nothingSelected() const {
    return (Mode() == eComponent && _countComponent.empty())
      || (Mode() == ePrimitive && _countPrimitive.empty());
}

void RadiantSelectionSystem::pivotChanged() const  {
    _pivotChanged = true;
    SceneChangeNotify();
}

// This gets called by the SelectionCounter, as this is the onChanged callback that got passed to it.
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

SelectionChangeCallback RadiantSelectionSystem::getObserver(EMode mode) {
	if (mode == ePrimitive) {
		return makeCallback1(_countPrimitive);
	}
    else {
		return makeCallback1(_countComponent);
	}
}

// return the number of selected primitives
std::size_t RadiantSelectionSystem::countSelected() const {
	return _countPrimitive.size();
}

// return the number of selected components
std::size_t RadiantSelectionSystem::countSelectedComponents() const {
	return _countComponent.size();
}

// This is called if the selection changes, so that the local list of selected instances can be updated
void RadiantSelectionSystem::onSelectedChanged(scene::Instance& instance, const Selectable& selectable) {
	// If the selectable is selected, add it to the local selection list, otherwise remove it 
	if (selectable.isSelected()) {
		_selection.append(instance);
	}
	else {
		_selection.erase(instance);
	}

	// Check if the number of selected primitives in the list matches the value of the selection counter
	ASSERT_MESSAGE(_selection.size() == _countPrimitive.size(), "selection-tracking error");
}

// greebo: This should be called "onComponentSelectionChanged", as it is a similar function of the above one
// Updates the internal list of component instances if the component selection gets changed
void RadiantSelectionSystem::onComponentSelection(scene::Instance& instance, const Selectable& selectable) {
	// If the instance got selected, add it to the list, otherwise remove it
	if (selectable.isSelected()) {
		_componentSelection.append(instance);
	}
    else {
		_componentSelection.erase(instance);
	}

	// Check if the number of selected components in the list matches the value of the selection counter 
	ASSERT_MESSAGE(_componentSelection.size() == _countComponent.size(), "selection-tracking error");
}

// Returns the last instance in the list (if the list is not empty)
scene::Instance& RadiantSelectionSystem::ultimateSelected() const {
	ASSERT_MESSAGE(_selection.size() > 0, "no instance selected");
	return _selection.back();
}

// Returns the instance before the last instance in the list (second from the end)
scene::Instance& RadiantSelectionSystem::penultimateSelected() const {
	ASSERT_MESSAGE(_selection.size() > 1, "only one instance selected");
	return *(*(--(--_selection.end())));
}

// Deselect or select all the instances in the scenegraph and notify the manipulator class as well
void RadiantSelectionSystem::setSelectedAll(bool selected) {
	GlobalSceneGraph().traverse(SelectAllWalker(selected));
	_manipulator->setSelected(selected);
}

// Deselect or select all the component instances in the scenegraph and notify the manipulator class as well
void RadiantSelectionSystem::setSelectedAllComponents(bool selected) {
	// Select all components in the scene, be it vertices, edges or faces
	GlobalSceneGraph().traverse(SelectAllComponentWalker(selected, SelectionSystem::eVertex));
	GlobalSceneGraph().traverse(SelectAllComponentWalker(selected, SelectionSystem::eEdge));
	GlobalSceneGraph().traverse(SelectAllComponentWalker(selected, SelectionSystem::eFace));
	
	_manipulator->setSelected(selected);
}

// Traverse the current selection and visit them with the given visitor class
void RadiantSelectionSystem::foreachSelected(const Visitor& visitor) const {
	SelectionListType::const_iterator i = _selection.begin();
	while(i != _selection.end()) {
		visitor.visit(*(*(i++)));
	}
}

// Traverse the current selection components and visit them with the given visitor class
void RadiantSelectionSystem::foreachSelectedComponent(const Visitor& visitor) const {
	SelectionListType::const_iterator i = _componentSelection.begin();
	while(i != _componentSelection.end()) {
		visitor.visit(*(*(i++)));
	}
}

// Add a "selection changed" callback
void RadiantSelectionSystem::addSelectionChangeCallback(const SelectionChangeHandler& handler) {
	_selectionChangedCallbacks.connectLast(handler);
}

// Call the "selection changed" callback with the selectable
void RadiantSelectionSystem::selectionChanged(const Selectable& selectable) {
	_selectionChangedCallbacks(selectable);
}

// Start a move, the current pivot point is saved as a start point
void RadiantSelectionSystem::startMove() {
	_pivot2worldStart = GetPivot2World();
}

/* greebo: This is called by the ManipulateObserver class on the mouseDown event. It checks, if a manipulator
 * can be selected where the mouse is pointing to.
 */
bool RadiantSelectionSystem::SelectManipulator(const View& view, const float device_point[2], const float device_epsilon[2]) {
	if (!nothingSelected() || (ManipulatorMode() == eDrag && Mode() == eComponent)) {
#if defined (DEBUG_SELECTION)
		g_render_clipped.destroy();
#endif

		// Unselect any currently selected manipulators to be sure
		_manipulator->setSelected(false);

		// Test, if the current manipulator can be selected
		if (!nothingSelected() || (ManipulatorMode() == eDrag && Mode() == eComponent)) {
			View scissored(view);
			ConstructSelectionTest(scissored, SelectionBoxForPoint(device_point, device_epsilon));
			
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

			_manip2pivotStart = matrix4_multiplied_by_matrix4(matrix4_full_inverse(_pivot2worldStart), pivot._worldSpace);

			Matrix4 device2manip;
			ConstructDevice2Manip(device2manip, _pivot2worldStart, view.GetModelview(), view.GetProjection(), view.GetViewport());
			_manipulator->GetManipulatable()->Construct(device2manip, device_point[0], device_point[1]);

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
void RadiantSelectionSystem::SelectPoint(const View& view, 
										 const float device_point[2], 
										 const float device_epsilon[2], 
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

#if defined (DEBUG_SELECTION)
	g_render_clipped.destroy();
#endif

	{
		View scissored(view);
		// Construct a selection test according to a small box with 2*epsilon edge length
		ConstructSelectionTest(scissored, SelectionBoxForPoint(device_point, device_epsilon));

		// Create a new SelectionPool instance and fill it with possible candidates
		SelectionVolume volume(scissored);
		// The possible candidates are stored in the SelectionPool
		SelectionPool selector;
		if (face) {
			Scene_TestSelect_Component(selector, volume, scissored, eFace);
		}
		else {
			Scene_TestSelect(selector, volume, scissored, Mode(), ComponentMode());
		}
		
		// Was the selection test successful (have we found anything to select)?
		if (!selector.failed()) {
			// Yes, now determine how we should interpret the click
			switch (modifier) {
				// If we are in toggle mode (Shift-Left-Click by default), just toggle the
				// selection of the "topmost" item
				case SelectionSystem::eToggle: {
					SelectableSortedSet::iterator best = selector.begin();
					// toggle selection of the object with least depth (=first in the list)
					if ((*best).second->isSelected())
						(*best).second->setSelected(false);
					else
						(*best).second->setSelected(true);
				}
				break;
				// greebo: eReplace mode gets active as soon as the user holds the replace modifiers down
				// and clicks (by default: Alt-Shift). eReplace is only active during the first click
				// afterwards we are in cycle mode.
				// if cycle mode not enabled, enable it
				case SelectionSystem::eReplace: {
					// select closest (=first in the list)
					(*selector.begin()).second->setSelected(true);
				}
				break;
				// select the next object in the list from the one already selected
				// greebo: eCycle is set if the user keeps holding the replace modifiers (Alt-Shift)
				// and does NOT move the mouse between the clicks, otherwise we fall back into eReplace mode
				// Note: The mode is set in SelectObserver::testSelect()
				case SelectionSystem::eCycle: {
					// Cycle through the selection pool and activate the item right after the currently selected
					SelectionPool::iterator i = selector.begin();
					while (i != selector.end()) {
						if ((*i).second->isSelected()) {
							// unselect the currently selected one
							(*i).second->setSelected(false);
							// check if there is a "next" item in the list, if not: select the first item 
							++i;
							if (i != selector.end()) {
								i->second->setSelected(true);
							}
							else {
								selector.begin()->second->setSelected(true);
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
void RadiantSelectionSystem::SelectArea(const View& view, 
										const float device_point[2], 
										const float device_delta[2], 
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

#if defined (DEBUG_SELECTION)
	g_render_clipped.destroy();
#endif

	{
		// Construct the selection test according to the area the user covered with his drag
		View scissored(view);
		ConstructSelectionTest(scissored, SelectionBoxForArea(device_point, device_delta));
		
		SelectionVolume volume(scissored);
		// The posssible candidates go here
		SelectionPool pool;
		if (face) {
			Scene_TestSelect_Component(pool, volume, scissored, eFace);
		}
		else {
			Scene_TestSelect(pool, volume, scissored, Mode(), ComponentMode());
		}
		
		// Cycle through the selection pool and toggle the candidates, but only if we are in toggle mode
		for (SelectionPool::iterator i = pool.begin(); i != pool.end(); ++i) {
			(*i).second->setSelected(!(modifier == SelectionSystem::eToggle && (*i).second->isSelected()));
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
		matrix4_translate_by_vec3(_pivot2world, translation);
		
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

			matrix4_assign_rotation_for_pivot(_pivot2world, _componentSelection.back());
		}
		else {
			Scene_Rotate_Selected(GlobalSceneGraph(), _rotation, _pivot2world.t().getVector3());

			matrix4_assign_rotation_for_pivot(_pivot2world, _selection.back());
		}

		// Update the views
		SceneChangeNotify();
	}
}

// Applies the scaling vector <scaling> to the current selection
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
void RadiantSelectionSystem::outputTranslation(TextOutputStream& ostream) {
	ostream << " -xyz " << _translation.x() << " " << _translation.y() << " " << _translation.z();
}

void RadiantSelectionSystem::outputRotation(TextOutputStream& ostream) {
	ostream << " -eulerXYZ " << _rotation.x() << " " << _rotation.y() << " " << _rotation.z();
}

void RadiantSelectionSystem::outputScale(TextOutputStream& ostream) {
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
void RadiantSelectionSystem::MoveSelected(const View& view, const float device_point[2]) {
	// Check, if the active manipulator is selected in the first place
	if (_manipulator->isSelected()) {
		// Initalise the undo system
		if (!_undoBegun) {
			_undoBegun = true;
			GlobalUndoSystem().start();
		}

		Matrix4 device2manip;
		ConstructDevice2Manip(device2manip, _pivot2worldStart, view.GetModelview(), view.GetProjection(), view.GetViewport());
		
		// Get the manipulatable from the currently active manipulator (done by selection test)
		// and call the Transform method (can be anything) 
		_manipulator->GetManipulatable()->Transform(_manip2pivotStart, device2manip, device_point[0], device_point[1]);
	}
}

/// \todo Support view-dependent nudge.
void RadiantSelectionSystem::NudgeManipulator(const Vector3& nudge, const Vector3& view) {
	if(ManipulatorMode() == eTranslate || ManipulatorMode() == eDrag) {
		translateSelected(nudge);
	}
}

// greebo: This just passes the call on to renderSolid, the manipulators are wireframes anyway 
void RadiantSelectionSystem::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	renderSolid(renderer, volume);
}

// Lets the ConstructPivot() method do the work and returns the result that is stored in the member variable 
const Matrix4& RadiantSelectionSystem::GetPivot2World() const {
	ConstructPivot();
	return _pivot2world;
}

void RadiantSelectionSystem::constructStatic() {
	_state = GlobalShaderCache().capture("$POINT");
#if defined(DEBUG_SELECTION)
	g_state_clipped = GlobalShaderCache().capture("$DEBUG_CLIPPED");
#endif
	TranslateManipulator::_stateWire = GlobalShaderCache().capture("$WIRE_OVERLAY");
	TranslateManipulator::_stateFill = GlobalShaderCache().capture("$FLATSHADE_OVERLAY");
	RotateManipulator::_stateOuter = GlobalShaderCache().capture("$WIRE_OVERLAY");
}

void RadiantSelectionSystem::destroyStatic() {
#if defined(DEBUG_SELECTION)
	GlobalShaderCache().release("$DEBUG_CLIPPED");
#endif
	GlobalShaderCache().release("$WIRE_OVERLAY");
	GlobalShaderCache().release("$FLATSHADE_OVERLAY");
	GlobalShaderCache().release("$WIRE_OVERLAY");
	GlobalShaderCache().release("$POINT");
}

void RadiantSelectionSystem::cancelMove() {

	// Unselect any currently selected manipulators to be sure
	_manipulator->setSelected(false);

	// Tell all the scene objects to revert their transformations
	GlobalSceneGraph().traverse(RevertTransforms());
	
	_pivotMoving = false;
	pivotChanged();
	
	// greebo: Deselect all faces if we are in brush and drag mode
	if (Mode() == ePrimitive) {
		if (ManipulatorMode() == eDrag) {
			GlobalSceneGraph().traverse(SelectAllComponentWalker(false, SelectionSystem::eFace));
			//Scene_SelectAll_Component(false, SelectionSystem::eFace);
		}
	}
	
	if (_undoBegun) {
		// Cancel the undo operation, if one has been begun 
		GlobalUndoSystem().cancel();
	}
	
	// Update the views
	SceneChangeNotify();
}

// This actually applies the transformation to the objects
void RadiantSelectionSystem::freezeTransforms() {
	GlobalSceneGraph().traverse(FreezeTransforms());
}

// End the move, this freezes the current transforms
void RadiantSelectionSystem::endMove() {
	freezeTransforms();

	// greebo: Deselect all faces if we are in brush and drag mode
	if (Mode() == ePrimitive) {
		if (ManipulatorMode() == eDrag) {
			GlobalSceneGraph().traverse(SelectAllComponentWalker(false, SelectionSystem::eFace));
			//Scene_SelectAll_Component(false, SelectionSystem::eFace);
		}
	}

	_pivotMoving = false;
	pivotChanged();

	// Update the views
	SceneChangeNotify();
	
	// If we started an undoable operation, end it now and tell the console what happened 
	if (_undoBegun) {
		StringOutputStream command;

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

		// Finish the undo move
		GlobalUndoSystem().finish(command.c_str());
	}
}

/* greebo: This calculates and constructs the pivot point of the selection.
 * It cycles through all selected objects and creates its AABB. The origin point of the AABB
 * is basically the pivot point. Pivot2World is therefore a translation from (0,0,0) to the calculated origin.
 *
 * The pivot point is also snapped to the grid.
 */
void RadiantSelectionSystem::ConstructPivot() const {
	if (!_pivotChanged || _pivotMoving)
		return;
		
	_pivotChanged = false;

	Vector3 objectPivot;

	if (!nothingSelected()) {
	    {
	    	// Create a local variable where the aabb information is stored
			AABB bounds;
			
			// Traverse through the selection and update the <bounds> variable
			if (Mode() == eComponent) {
				GlobalSceneGraph().traverse(BoundsSelectedComponent(bounds));
			}
			else {
				GlobalSceneGraph().traverse(BoundsSelected(bounds));
			}
			// the <bounds> variable now contains the AABB of the selection, retrieve the origin
			objectPivot = bounds.origin;
		}

		// Snap the pivot point to the grid
		vector3_snap(objectPivot, GlobalGrid().getGridSize());
		
		// The pivot2world matrix is just a translation from the world origin (0,0,0) to the object pivot  
		_pivot2world = Matrix4::getTranslation(objectPivot);

		// Only rotation and scaling need further calculations  
		switch (_manipulatorMode) {
			case eTranslate:
				break;
			case eRotate:
				if (Mode() == eComponent) {
					matrix4_assign_rotation_for_pivot(_pivot2world, _componentSelection.back());
				}
				else {
					matrix4_assign_rotation_for_pivot(_pivot2world, _selection.back());
				}
				break;
		    case eScale:
				if (Mode() == eComponent) {
					matrix4_assign_rotation_for_pivot(_pivot2world, _componentSelection.back());
				}
				else {
					matrix4_assign_rotation_for_pivot(_pivot2world, _selection.back());
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
void RadiantSelectionSystem::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	if (!nothingSelected()) {
		renderer.Highlight(Renderer::ePrimitive, false);
		renderer.Highlight(Renderer::eFace, false);

		renderer.SetState(_state, Renderer::eWireframeOnly);
		renderer.SetState(_state, Renderer::eFullMaterials);

		_manipulator->render(renderer, volume, GetPivot2World());
	}

#if defined(DEBUG_SELECTION)
	renderer.SetState(g_state_clipped, Renderer::eWireframeOnly);
	renderer.SetState(g_state_clipped, Renderer::eFullMaterials);
	renderer.addRenderable(g_render_clipped, g_render_clipped.m_world);
#endif
}

