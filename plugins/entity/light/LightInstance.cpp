#include "LightInstance.h"
#include "qerplugin.h"
#include "../../../radiant/ui/colourscheme/ColourSchemeManager.h"

// ------ LightInstance class implementation ----------------------------------

// Constructor
LightInstance::LightInstance(const scene::Path& path, scene::Instance* parent, Light& contained) :
	TargetableInstance(path, parent, this, StaticTypeCasts::instance().get(), contained.getEntity(), *this),
	TransformModifier(Light::TransformChangedCaller(contained), ApplyTransformCaller(*this)),
	_light(contained),
	_lightCenterInstance(VertexInstance(_light.getDoom3Radius().m_centerTransformed, SelectedChangedComponentCaller(*this))),
	m_dragPlanes(SelectedChangedComponentCaller(*this))
{
	_light.instanceAttach(Instance::path());    

	if (g_lightType == LIGHTTYPE_DOOM3) {
		GlobalShaderCache().attach(*this);
		// greebo: Connect the lightChanged() member method to the "light changed" callback
		_light.setLightChangedCallback(LightChangedCaller(*this));
	}

	StaticRenderableConnectionLines::instance().attach(*this);
}

// Destructor
LightInstance::~LightInstance() {
	StaticRenderableConnectionLines::instance().detach(*this);

	if (g_lightType == LIGHTTYPE_DOOM3) {
		_light.setLightChangedCallback(Callback());
		GlobalShaderCache().detach(*this);
	}

	_light.instanceDetach(Instance::path());
}

Bounded& LightInstance::get(NullType<Bounded>) {
	return _light;
}

void LightInstance::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	const bool lightIsSelected = getSelectable().isSelected();
	_light.renderSolid(renderer, volume, Instance::localToWorld(), lightIsSelected);
	
	// We are not in component selection mode, check if we should draw the center of the light anyway
	if (lightIsSelected && 
		GlobalSelectionSystem().ComponentMode() != SelectionSystem::eVertex &&
	    GlobalRegistry().get("user/ui/alwaysShowLightCenter") == "1") 
	{
		_light.getDoom3Radius().setCenterColour(GlobalRadiant().getColour("light_center_normal"));
		_light.renderLightCentre(renderer, volume, Instance::localToWorld());
	}
}
  
void LightInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	const bool lightIsSelected = getSelectable().isSelected();
	_light.renderWireframe(renderer, volume, Instance::localToWorld(), lightIsSelected);
	
	// greebo: We are not in component selection mode (and the light is still selected), 
	// check if we should draw the center of the light anyway
	if (lightIsSelected && 
		GlobalSelectionSystem().ComponentMode() != SelectionSystem::eVertex &&
	    GlobalRegistry().get("user/ui/alwaysShowLightCenter") == "1") 
	{
		_light.getDoom3Radius().setCenterColour(GlobalRadiant().getColour("light_center_normal"));
		_light.renderLightCentre(renderer, volume, Instance::localToWorld());
	}
}

// Renders the components of this light instance 
void LightInstance::renderComponents(Renderer& renderer, const VolumeTest& volume) const {
	
	// Render the components (light center) as selected/deselected, if we are in the according mode
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex) {
		// Update the colour of the light center dot 
		if (_lightCenterInstance.isSelected()) {
			_light.getDoom3Radius().setCenterColour(GlobalRadiant().getColour("light_center_selected"));
			_light.renderLightCentre(renderer, volume, Instance::localToWorld());
		}
		else {
			_light.getDoom3Radius().setCenterColour(GlobalRadiant().getColour("light_center_deselected"));
			_light.renderLightCentre(renderer, volume, Instance::localToWorld());
		}
	}
}

const AABB& LightInstance::getSelectedComponentsBounds() const {
	// Create a new axis aligned bounding box
	m_aabb_component = AABB();

	m_aabb_component.includePoint(_lightCenterInstance.getVertex());

	return m_aabb_component;
}

// Test the light volume for selection, this just passes the call on to the contained Light class
void LightInstance::testSelect(Selector& selector, SelectionTest& test) {
	_light.testSelect(selector, test, Instance::localToWorld());
}

void LightInstance::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());
	// greebo: Make sure to use the lightAABB() for the selection test, excluding the light center
	m_dragPlanes.selectPlanes(_light.lightAABB(), selector, test, selectedPlaneCallback, rotation());
}

void LightInstance::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	// greebo: Make sure to use the lightAABB() for the selection test, excluding the light center
	m_dragPlanes.selectReversedPlanes(_light.lightAABB(), selector, selectedPlanes, rotation());
}
 
// greebo: Returns true if drag planes or the light center is selected (both are components)
bool LightInstance::isSelectedComponents() const {
	return (m_dragPlanes.isSelected() || _lightCenterInstance.isSelected());
}

// greebo: Selects/deselects all components, depending on the chosen componentmode
void LightInstance::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eFace) {
		m_dragPlanes.setSelected(false);
	}
    
	if (mode == SelectionSystem::eVertex) {
		_lightCenterInstance.setSelected(false);
	}
}

void LightInstance::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	// Get the Origin of the Light Volume AABB (NOT the localAABB() which includes the light center)
	Vector3 lightOrigin = _light.lightAABB().origin;
  	
	Matrix4 local2World = Matrix4::getTranslation(lightOrigin);
  	
	test.BeginMesh(local2World);
  	
	if (mode == SelectionSystem::eVertex) {
		// Test if the light center is hit by the click 
		_lightCenterInstance.testSelect(selector, test);
	}
}

void LightInstance::selectedChangedComponent(const Selectable& selectable) {
	// Call the selection observer for components. This triggers the selectionChangeCallback of the selection system
	GlobalSelectionSystem().getObserver(SelectionSystem::eComponent)(selectable);
	// add the selectable to the list of selected components (see RadiantSelectionSystem::onComponentSelection)
	GlobalSelectionSystem().onComponentSelection(*this, selectable);
}

void LightInstance::evaluateTransform() {
	if (getType() == TRANSFORM_PRIMITIVE) {
		_light.translate(getTranslation());
		_light.rotate(getRotation());
	}
	else {
		// Check if the light center is selected, if yes, transform it, if not, it's a drag plane operation 
		if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex 
			&& _lightCenterInstance.isSelected()) 
		{
			// Retrieve the translation and apply it to the temporary light center variable
			// This adds the translation vector to the previous light origin 
			_light.getDoom3Radius().m_centerTransformed = _light.getDoom3Radius().m_center + getTranslation();
			
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

void LightInstance::applyTransform() {
	_light.revertTransform();
	evaluateTransform();
	_light.freezeTransform();
}

/*greebo: This is a callback function that gets connected in the constructor
* Don't know exactly what it does, but it seems to notify the shader cache that the light has moved or
* something like that.
*/ 
void LightInstance::lightChanged() {
	GlobalShaderCache().changed(*this);
}

Shader* LightInstance::getShader() const {
	return _light.getShader();
}

const AABB& LightInstance::aabb() const {
	return _light.aabb();
}

bool LightInstance::testAABB(const AABB& other) const {
	return _light.testAABB(other);
}

const Matrix4& LightInstance::rotation() const {
	return _light.rotation();
}

const Vector3& LightInstance::offset() const {
	return _light.offset();
}

const Vector3& LightInstance::colour() const {
	return _light.colour();
}

bool LightInstance::isProjected() const {
	return _light.isProjected();
}

const Matrix4& LightInstance::projection() const {
	return _light.projection();
}
