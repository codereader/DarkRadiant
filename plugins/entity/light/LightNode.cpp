#include "LightNode.h"

namespace entity {

// --------- LightNode implementation ------------------------------------

LightNode::LightNode(IEntityClassPtr eclass) :
	EntityNode(eclass),
	TransformModifier(Light::TransformChangedCaller(_light), ApplyTransformCaller(*this)),
	TargetableNode(_entity, *this),
	_light(eclass, 
			*this, 
			Node::TransformChangedCaller(*this), 
			Node::BoundsChangedCaller(*this), 
			EvaluateTransformCaller(*this)),
	_lightCenterInstance(VertexInstance(_light.getDoom3Radius().m_centerTransformed, SelectedChangedComponentCaller(*this))),
	_lightTargetInstance(VertexInstance(_light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightRightInstance(VertexInstanceRelative(_light.rightTransformed(), _light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightUpInstance(VertexInstanceRelative(_light.upTransformed(), _light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightStartInstance(VertexInstance(_light.startTransformed(), SelectedChangedComponentCaller(*this))),
	_lightEndInstance(VertexInstance(_light.endTransformed(), SelectedChangedComponentCaller(*this))),
	m_dragPlanes(SelectedChangedComponentCaller(*this))
{
	TargetableNode::construct();

	// greebo: Connect the lightChanged() member method to the "light changed" callback
	_light.setLightChangedCallback(LightChangedCaller(*this));
	GlobalShaderCache().attach(*this);
}

LightNode::LightNode(const LightNode& other) :
	EntityNode(other),
	SelectableNode(other),
	scene::Cloneable(other),
	Nameable(other),
	Snappable(other),
	Editable(other),
	TransformNode(other),
	SelectionTestable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	PlaneSelectable(other),
	Renderable(other),
	Bounded(other),
	TransformModifier(Light::TransformChangedCaller(_light), ApplyTransformCaller(*this)),
	RendererLight(other),
	scene::SelectableLight(other),
	TargetableNode(_entity, *this),
	_light(other._light, 
				*this, 
				Node::TransformChangedCaller(*this), 
				Node::BoundsChangedCaller(*this), 
				EvaluateTransformCaller(*this)),
	_lightCenterInstance(VertexInstance(_light.getDoom3Radius().m_centerTransformed, SelectedChangedComponentCaller(*this))),
	_lightTargetInstance(VertexInstance(_light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightRightInstance(VertexInstanceRelative(_light.rightTransformed(), _light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightUpInstance(VertexInstanceRelative(_light.upTransformed(), _light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightStartInstance(VertexInstance(_light.startTransformed(), SelectedChangedComponentCaller(*this))),
	_lightEndInstance(VertexInstance(_light.endTransformed(), SelectedChangedComponentCaller(*this))),
	m_dragPlanes(SelectedChangedComponentCaller(*this))
{
	TargetableNode::construct();

	// greebo: Connect the lightChanged() member method to the "light changed" callback
	_light.setLightChangedCallback(LightChangedCaller(*this));
	GlobalShaderCache().attach(*this);
}

LightNode::~LightNode() {
	GlobalShaderCache().detach(*this);
	_light.setLightChangedCallback(Callback());

	TargetableNode::destruct();
}

const Matrix4& LightNode::getLocalPivot() const {
	return _light.getLocalPivot();
}

// Snappable implementation
void LightNode::snapto(float snap) {
	_light.snapto(snap);
}

AABB LightNode::getSelectAABB() {
	AABB returnValue = _light.lightAABB();
	
	default_extents(returnValue.extents);
	
	return returnValue;
}

/*greebo: This is a callback function that gets connected in the constructor
* Don't know exactly what it does, but it seems to notify the shader cache that the light has moved or
* something like that.
*/ 
void LightNode::lightChanged() {
	GlobalShaderCache().changed(*this);
}

// TransformNode implementation
const Matrix4& LightNode::localToParent() const {
	return _light.getTransformNode().localToParent();
}

Entity& LightNode::getEntity() {
	return _light.getEntity();
}

void LightNode::refreshModel() {
	// Nothing to do
}

const AABB& LightNode::localAABB() const {
	return _light.localAABB();
}

void LightNode::onRemoveFromScene() {
	// Call the base class first
	SelectableNode::onRemoveFromScene();

	// De-select all child components as well
	setSelectedComponents(false, SelectionSystem::eVertex);
	setSelectedComponents(false, SelectionSystem::eFace);
}

// Test the light volume for selection, this just passes the call on to the contained Light class
void LightNode::testSelect(Selector& selector, SelectionTest& test) {
	_light.testSelect(selector, test, localToWorld());
}

// greebo: Returns true if drag planes or one or more light vertices are selected
bool LightNode::isSelectedComponents() const {
	return (m_dragPlanes.isSelected() || _lightCenterInstance.isSelected() ||
			_lightTargetInstance.isSelected() || _lightRightInstance.isSelected() ||
			_lightUpInstance.isSelected() || _lightStartInstance.isSelected() ||
			_lightEndInstance.isSelected() );
}

// greebo: Selects/deselects all components, depending on the chosen componentmode
void LightNode::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eFace) {
		m_dragPlanes.setSelected(false);
	}
    
	if (mode == SelectionSystem::eVertex) {
		_lightCenterInstance.setSelected(false);
		_lightTargetInstance.setSelected(false);
		_lightRightInstance.setSelected(false);
		_lightUpInstance.setSelected(false);
		_lightStartInstance.setSelected(false);
		_lightEndInstance.setSelected(false);
	}
}

void LightNode::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	// Get the Origin of the Light Volume AABB (NOT the localAABB() which includes the light center)
	Vector3 lightOrigin = _light.lightAABB().origin;
  	
	Matrix4 local2World = Matrix4::getTranslation(lightOrigin);
  	
	test.BeginMesh(local2World);
  	
	if (mode == SelectionSystem::eVertex) {
		if (_light.isProjected()) {
			// Test the projection components for selection
			_lightTargetInstance.testSelect(selector, test);
			_lightRightInstance.testSelect(selector, test);
			_lightUpInstance.testSelect(selector, test);
			_lightStartInstance.testSelect(selector, test);
			_lightEndInstance.testSelect(selector, test);
		}
		else {
			// Test if the light center is hit by the click 
			_lightCenterInstance.testSelect(selector, test);
		}
	}
}

const AABB& LightNode::getSelectedComponentsBounds() const {
	// Create a new axis aligned bounding box
	m_aabb_component = AABB();

	if (_light.isProjected()) {
		// Include the according vertices in the AABB
		m_aabb_component.includePoint(_lightTargetInstance.getVertex());
		m_aabb_component.includePoint(_lightRightInstance.getVertex());
		m_aabb_component.includePoint(_lightUpInstance.getVertex());
		m_aabb_component.includePoint(_lightStartInstance.getVertex());
		m_aabb_component.includePoint(_lightEndInstance.getVertex());
	}
	else {
		// Just include the light center, this is the only vertex that may be out of the light volume
		m_aabb_component.includePoint(_lightCenterInstance.getVertex());
	}

	return m_aabb_component;
}

void LightNode::snapComponents(float snap) {
	if (_light.isProjected()) {
		// Check, if any components are selected and snap the selected ones to the grid
		if (isSelectedComponents()) {
			if (_lightTargetInstance.isSelected()) {
				vector3_snap(_light.targetTransformed(), snap);
			}
			if (_lightRightInstance.isSelected()) {
				vector3_snap(_light.rightTransformed(), snap);
			}
			if (_lightUpInstance.isSelected()) {
				vector3_snap(_light.upTransformed(), snap);
			}
			
			if (_light.useStartEnd()) {
				if (_lightEndInstance.isSelected()) {
					vector3_snap(_light.endTransformed(), snap);
				}
				
				if (_lightStartInstance.isSelected()) {
					vector3_snap(_light.startTransformed(), snap);
				}
			}
		}
		else {
			// None are selected, snap them all
			vector3_snap(_light.targetTransformed(), snap);
			vector3_snap(_light.rightTransformed(), snap);
			vector3_snap(_light.upTransformed(), snap);
			
			if (_light.useStartEnd()) {
				vector3_snap(_light.endTransformed(), snap);
				vector3_snap(_light.startTransformed(), snap);
			}
		}
	}
	else {
		// There is only one vertex for point lights, namely the light_center, always snap it 
		vector3_snap(_light.getDoom3Radius().m_centerTransformed, snap);
	}

	_light.freezeTransform();
}

void LightNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());
	// greebo: Make sure to use the lightAABB() for the selection test, excluding the light center
	m_dragPlanes.selectPlanes(_light.lightAABB(), selector, test, selectedPlaneCallback, rotation());
}

void LightNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	// greebo: Make sure to use the lightAABB() for the selection test, excluding the light center
	m_dragPlanes.selectReversedPlanes(_light.lightAABB(), selector, selectedPlanes, rotation());
}

scene::INodePtr LightNode::clone() const {
	scene::INodePtr clone(new LightNode(*this));
	clone->setSelf(clone);
	return clone;
}

void LightNode::instantiate(const scene::Path& path) {
	_light.instanceAttach(path);
	Node::instantiate(path);
}

void LightNode::uninstantiate(const scene::Path& path) {
	_light.instanceDetach(path);
	Node::uninstantiate(path);
}

// Nameable implementation
std::string LightNode::name() const {
	return _light.getNameable().name();
}

void LightNode::attach(const NameCallback& callback) {
	_light.getNameable().attach(callback);
}

void LightNode::detach(const NameCallback& callback) {
	_light.getNameable().detach(callback);
}

void LightNode::selectedChangedComponent(const Selectable& selectable) {
	// add the selectable to the list of selected components (see RadiantSelectionSystem::onComponentSelection)
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

/* greebo: This is the method that gets called by renderer.h. It passes the call 
 * on to the Light class render methods. 
 */
void LightNode::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	// Pass through to wireframe render
	renderWireframe(renderer, volume);
}
  
void LightNode::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	const bool lightIsSelected = isSelected();
	_light.renderWireframe(
		renderer, volume, localToWorld(), lightIsSelected
	);
	
	renderInactiveComponents(renderer, volume, lightIsSelected);
}

// Renders the components of this light instance 
void LightNode::renderComponents(Renderer& renderer, const VolumeTest& volume) const {
	// Render the components (light center) as selected/deselected, if we are in the according mode
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex) {
		if (_light.isProjected()) {
			// A projected light
			// Cache registry values to reduce number of queries
			Vector3 colourStartEndSelected = ColourSchemes().getColour("light_startend_selected");
			Vector3 colourStartEndDeselected = ColourSchemes().getColour("light_startend_deselected");
			Vector3 colourVertexSelected = ColourSchemes().getColour("light_vertex_selected");
			Vector3 colourVertexDeselected = ColourSchemes().getColour("light_vertex_deselected");
			
			// Update the colour of the light center dot
			const_cast<Light&>(_light).colourLightTarget() = (_lightTargetInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;
			const_cast<Light&>(_light).colourLightRight() = (_lightRightInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;
			const_cast<Light&>(_light).colourLightUp() = (_lightUpInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;
				
			const_cast<Light&>(_light).colourLightStart() = (_lightStartInstance.isSelected()) ? colourStartEndSelected : colourStartEndDeselected;
			const_cast<Light&>(_light).colourLightEnd() = (_lightEndInstance.isSelected()) ? colourStartEndSelected : colourStartEndDeselected;
			
			// Render the projection points
			_light.renderProjectionPoints(renderer, volume, localToWorld());
		}
		else {
			// A point light
			
			// Update the colour of the light center dot 
			if (_lightCenterInstance.isSelected()) {
				const_cast<Light&>(_light).getDoom3Radius().setCenterColour(ColourSchemes().getColour("light_vertex_selected"));
				_light.renderLightCentre(renderer, volume, localToWorld());
			}
			else {
				const_cast<Light&>(_light).getDoom3Radius().setCenterColour(ColourSchemes().getColour("light_vertex_deselected"));
				_light.renderLightCentre(renderer, volume, localToWorld());
			}
		}
	}
}

void LightNode::renderInactiveComponents(Renderer& renderer, const VolumeTest& volume, const bool selected) const {
	// greebo: We are not in component selection mode (and the light is still selected), 
	// check if we should draw the center of the light anyway
	if (selected 
		&& GlobalSelectionSystem().ComponentMode() != SelectionSystem::eVertex
		&& GlobalRegistry().get("user/ui/alwaysShowLightVertices") == "1") 
	{
		if (_light.isProjected()) {
			// Cache registry values to reduce number of queries
			Vector3 colourStartEndInactive = ColourSchemes().getColour("light_startend_deselected");
			Vector3 colourVertexInactive = ColourSchemes().getColour("light_vertex_normal");
			
			const_cast<Light&>(_light).colourLightStart() = colourStartEndInactive;
			const_cast<Light&>(_light).colourLightEnd() = colourStartEndInactive;
			const_cast<Light&>(_light).colourLightTarget() = colourVertexInactive;
			const_cast<Light&>(_light).colourLightRight() = colourVertexInactive;
			const_cast<Light&>(_light).colourLightUp() = colourVertexInactive;
			
			// Render the projection points
			_light.renderProjectionPoints(renderer, volume, localToWorld());
		} 
		else {
			const_cast<Light&>(_light).getDoom3Radius().setCenterColour(ColourSchemes().getColour("light_vertex_normal"));
			_light.renderLightCentre(renderer, volume, localToWorld());
		}
	}
}

void LightNode::evaluateTransform() {
	if (getType() == TRANSFORM_PRIMITIVE) {
		_light.translate(getTranslation());
		_light.rotate(getRotation());
	}
	else {
		// Check if the light center is selected, if yes, transform it, if not, it's a drag plane operation 
		if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex) {
			if (_lightCenterInstance.isSelected()) {
				// Retrieve the translation and apply it to the temporary light center variable
				// This adds the translation vector to the previous light origin 
				_light.getDoom3Radius().m_centerTransformed = 
										_light.getDoom3Radius().m_center + getTranslation();
			}
			
			if (_lightTargetInstance.isSelected()) {
				// Delegate the work to the Light class
				_light.translateLightTarget(getTranslation());
			}
			
			if (_lightRightInstance.isSelected()) {
				// Save the new light_right vector
				_light.rightTransformed() = _light.right() + getTranslation();
			}
			
			if (_lightUpInstance.isSelected()) {
				// Save the new light_up vector
				_light.upTransformed() = _light.up() + getTranslation();
			}
			
			if (_lightStartInstance.isSelected()) {
				// Delegate the work to the Light class (including boundary checks)
				_light.translateLightStart(getTranslation());
			}
			
			if (_lightEndInstance.isSelected()) {
				// Save the new light_up vector
				_light.endTransformed() = _light.end() + getTranslation();
			}
			
			// If this is a projected light, then it is likely for the according vertices to have changed, so update the projection
			if (_light.isProjected()) {
				// Call projection changed, so that the recalculation can be triggered (call for projection() would be ignored otherwise)
				_light.projectionChanged();
				
				// Recalculate the frustum
				_light.projection();
			}
		}
		else {
			// Ordinary Drag manipulator
			//m_dragPlanes.m_bounds = _light.aabb();
			// greebo: Be sure to use the actual lightAABB for evaluating the drag operation, NOT
			// the aabb() or localABB() method, that returns the bounding box including the light center,
			// which may be positioned way out of the volume
			m_dragPlanes.m_bounds = _light.lightAABB();
			_light.setLightRadius(m_dragPlanes.evaluateResize(getTranslation(), rotation()));
		}
	}
}

void LightNode::applyTransform() {
	_light.revertTransform();
	evaluateTransform();
	_light.freezeTransform();
}


Vector3 LightNode::worldOrigin() const
{
    return _light.getOrigin();
}

ShaderPtr LightNode::getShader() const {
	return _light.getShader();
}

const AABB& LightNode::aabb() const {
	return _light.aabb();
}

bool LightNode::testAABB(const AABB& other) const {
	return _light.testAABB(other);
}

const Vector3& LightNode::offset() const {
	return _light.offset();
}

const Vector3& LightNode::colour() const {
	return _light.colour();
}

bool LightNode::isProjected() const {
	return _light.isProjected();
}

const Matrix4& LightNode::projection() const {
	return _light.projection();
}

const Matrix4& LightNode::rotation() const {
	return _light.rotation();
}

} // namespace entity
