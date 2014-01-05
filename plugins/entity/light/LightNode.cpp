#include "LightNode.h"

#include "../EntitySettings.h"
#include <boost/bind.hpp>

namespace entity {

// --------- LightNode implementation ------------------------------------

LightNode::LightNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	_light(_entity,
		   *this,
		   Callback(boost::bind(&scene::Node::transformChanged, this)),
		   Callback(boost::bind(&scene::Node::boundsChanged, this)),
		   Callback(boost::bind(&LightNode::lightChanged, this))),
	_lightCenterInstance(_light.getDoom3Radius().m_centerTransformed, boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightTargetInstance(_light.targetTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightRightInstance(_light.rightTransformed(), _light.targetTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightUpInstance(_light.upTransformed(), _light.targetTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightStartInstance(_light.startTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightEndInstance(_light.endTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	m_dragPlanes(boost::bind(&LightNode::selectedChangedComponent, this, _1))
{}

LightNode::LightNode(const LightNode& other) :
	EntityNode(other),
	ILightNode(other),
	_light(other._light,
		   *this,
           _entity,
           Callback(boost::bind(&Node::transformChanged, this)),
		   Callback(boost::bind(&Node::boundsChanged, this)),
		   Callback(boost::bind(&LightNode::lightChanged, this))),
	_lightCenterInstance(_light.getDoom3Radius().m_centerTransformed, boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightTargetInstance(_light.targetTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightRightInstance(_light.rightTransformed(), _light.targetTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightUpInstance(_light.upTransformed(), _light.targetTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightStartInstance(_light.startTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	_lightEndInstance(_light.endTransformed(), boost::bind(&LightNode::selectedChangedComponent, this, _1)),
	m_dragPlanes(boost::bind(&LightNode::selectedChangedComponent, this, _1))
{}

LightNodePtr LightNode::Create(const IEntityClassPtr& eclass)
{
	LightNodePtr instance(new LightNode(eclass));
	instance->construct();

	return instance;
}

void LightNode::construct()
{
	EntityNode::construct();

	_light.construct();
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
	GlobalRenderSystem().lightChanged(*this);
}

const AABB& LightNode::localAABB() const {
	return _light.localAABB();
}

float LightNode::getShaderParm(int parmNum) const
{
	return EntityNode::getShaderParm(parmNum);
}

void LightNode::onInsertIntoScene()
{
	// Call the base class first
	EntityNode::onInsertIntoScene();

	GlobalRenderSystem().attachLight(*this);
}

void LightNode::onRemoveFromScene()
{
	// Call the base class first
	EntityNode::onRemoveFromScene();

	GlobalRenderSystem().detachLight(*this);

	// De-select all child components as well
	setSelectedComponents(false, SelectionSystem::eVertex);
	setSelectedComponents(false, SelectionSystem::eFace);
}

void LightNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

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
				_light.targetTransformed().snap(snap);
			}
			if (_lightRightInstance.isSelected()) {
				_light.rightTransformed().snap(snap);
			}
			if (_lightUpInstance.isSelected()) {
				_light.upTransformed().snap(snap);
			}

			if (_light.useStartEnd()) {
				if (_lightEndInstance.isSelected()) {
					_light.endTransformed().snap(snap);
				}

				if (_lightStartInstance.isSelected()) {
					_light.startTransformed().snap(snap);
				}
			}
		}
		else {
			// None are selected, snap them all
			_light.targetTransformed().snap(snap);
			_light.rightTransformed().snap(snap);
			_light.upTransformed().snap(snap);

			if (_light.useStartEnd()) {
				_light.endTransformed().snap(snap);
				_light.startTransformed().snap(snap);
			}
		}
	}
	else {
		// There is only one vertex for point lights, namely the light_center, always snap it
		_light.getDoom3Radius().m_centerTransformed.snap(snap);
	}

	_light.freezeTransform();
}

void LightNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());
	// greebo: Make sure to use the local lightAABB() for the selection test, excluding the light center
	AABB localLightAABB(Vector3(0,0,0), _light.getDoom3Radius().m_radiusTransformed);
	m_dragPlanes.selectPlanes(localLightAABB, selector, test, selectedPlaneCallback, rotation());
}

void LightNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
{
	AABB localLightAABB(Vector3(0,0,0), _light.getDoom3Radius().m_radiusTransformed);
	m_dragPlanes.selectReversedPlanes(localLightAABB, selector, selectedPlanes, rotation());
}

scene::INodePtr LightNode::clone() const
{
	LightNodePtr node(new LightNode(*this));
	node->construct();

	return node;
}

void LightNode::selectedChangedComponent(const Selectable& selectable) {
	// add the selectable to the list of selected components (see RadiantSelectionSystem::onComponentSelection)
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

/* greebo: This is the method that gets called by renderer.h. It passes the call
 * on to the Light class render methods.
 */
void LightNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	// Pass through to wireframe render
	renderWireframe(collector, volume);
}

void LightNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	const bool lightIsSelected = isSelected();
	_light.renderWireframe(
		collector, volume, localToWorld(), lightIsSelected
	);

	renderInactiveComponents(collector, volume, lightIsSelected);
}

void LightNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

	// The renderable vertices are maintaining shader objects, acquire/free them now
	_light.setRenderSystem(renderSystem);

	_lightCenterInstance.setRenderSystem(renderSystem);
	_lightTargetInstance.setRenderSystem(renderSystem);
	_lightRightInstance.setRenderSystem(renderSystem);
	_lightUpInstance.setRenderSystem(renderSystem);
	_lightStartInstance.setRenderSystem(renderSystem);
	_lightEndInstance.setRenderSystem(renderSystem);
}

// Renders the components of this light instance
void LightNode::renderComponents(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Render the components (light center) as selected/deselected, if we are in the according mode
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex)
	{
		if (_light.isProjected())
		{
			// A projected light
			
			EntitySettings& settings = *EntitySettings::InstancePtr();

			const Vector3& colourStartEndSelected = settings.getLightVertexColour(EntitySettings::VERTEX_START_END_SELECTED);
			const Vector3& colourStartEndDeselected = settings.getLightVertexColour(EntitySettings::VERTEX_START_END_DESELECTED);
			const Vector3& colourVertexSelected = settings.getLightVertexColour(EntitySettings::VERTEX_SELECTED);
			const Vector3& colourVertexDeselected = settings.getLightVertexColour(EntitySettings::VERTEX_DESELECTED);

			// Update the colour of the light center dot
			const_cast<Light&>(_light).colourLightTarget() = (_lightTargetInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;
			const_cast<Light&>(_light).colourLightRight() = (_lightRightInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;
			const_cast<Light&>(_light).colourLightUp() = (_lightUpInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;

			const_cast<Light&>(_light).colourLightStart() = (_lightStartInstance.isSelected()) ? colourStartEndSelected : colourStartEndDeselected;
			const_cast<Light&>(_light).colourLightEnd() = (_lightEndInstance.isSelected()) ? colourStartEndSelected : colourStartEndDeselected;

			// Render the projection points
			_light.renderProjectionPoints(collector, volume, localToWorld());
		}
		else
		{
			// A point light

			// Update the colour of the light center dot
			if (_lightCenterInstance.isSelected())
			{
				const_cast<Light&>(_light).getDoom3Radius().setCenterColour(
					EntitySettings::InstancePtr()->getLightVertexColour(EntitySettings::VERTEX_SELECTED));
				_light.renderLightCentre(collector, volume, localToWorld());
			}
			else
			{
				const_cast<Light&>(_light).getDoom3Radius().setCenterColour(
					EntitySettings::InstancePtr()->getLightVertexColour(EntitySettings::VERTEX_DESELECTED));
				_light.renderLightCentre(collector, volume, localToWorld());
			}
		}
	}
}

void LightNode::renderInactiveComponents(RenderableCollector& collector, const VolumeTest& volume, const bool selected) const
{
	// greebo: We are not in component selection mode (and the light is still selected),
	// check if we should draw the center of the light anyway
	if (selected
		&& GlobalSelectionSystem().ComponentMode() != SelectionSystem::eVertex
		&& EntitySettings::InstancePtr()->alwaysShowLightVertices())
	{
		if (_light.isProjected())
		{
			EntitySettings& settings = *EntitySettings::InstancePtr();
			const Vector3& colourStartEndInactive = settings.getLightVertexColour(EntitySettings::VERTEX_START_END_DESELECTED);
			const Vector3& colourVertexInactive = settings.getLightVertexColour(EntitySettings::VERTEX_DESELECTED);

			const_cast<Light&>(_light).colourLightStart() = colourStartEndInactive;
			const_cast<Light&>(_light).colourLightEnd() = colourStartEndInactive;
			const_cast<Light&>(_light).colourLightTarget() = colourVertexInactive;
			const_cast<Light&>(_light).colourLightRight() = colourVertexInactive;
			const_cast<Light&>(_light).colourLightUp() = colourVertexInactive;

			// Render the projection points
			_light.renderProjectionPoints(collector, volume, localToWorld());
		}
		else
		{
			const Vector3& colourVertexInactive = EntitySettings::InstancePtr()->getLightVertexColour(
				EntitySettings::VERTEX_INACTIVE);

			const_cast<Light&>(_light).getDoom3Radius().setCenterColour(colourVertexInactive);
			_light.renderLightCentre(collector, volume, localToWorld());
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

Vector3 LightNode::worldOrigin() const
{
    return _light.worldOrigin();
}

Matrix4 LightNode::getLightTextureTransformation() const
{
    return _light.getLightTextureTransformation();
}

ShaderPtr LightNode::getShader() const {
	return _light.getShader();
}

bool LightNode::intersectsAABB(const AABB& aabb) const
{
	return _light.intersectsAABB(aabb);
}

Vector3 LightNode::getLightOrigin() const {
	return _light.getLightOrigin();
}

const Matrix4& LightNode::rotation() const {
	return _light.rotation();
}

void LightNode::_onTransformationChanged()
{
	_light.revertTransform();
	evaluateTransform();
	_light.updateOrigin();
}

void LightNode::_applyTransformation()
{
	_light.revertTransform();
	evaluateTransform();
	_light.freezeTransform();
}

} // namespace entity
