#include "LightInstance.h"
#include "qerplugin.h"
#include "math/pi.h"
#include "../../../radiant/ui/colourscheme/ColourSchemeManager.h"

// ------ LightInstance class implementation ----------------------------------

// Constructor
LightInstance::LightInstance(const scene::Path& path, scene::Instance* parent, Light& contained) :
	TargetableInstance(path, parent, this, StaticTypeCasts::instance().get(), contained.getEntity(), *this),
	TransformModifier(Light::TransformChangedCaller(contained), ApplyTransformCaller(*this)),
	_light(contained),
	_lightCenterInstance(VertexInstance(_light.getDoom3Radius().m_centerTransformed, SelectedChangedComponentCaller(*this))),
	_lightTargetInstance(VertexInstance(_light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightRightInstance(VertexInstanceRelative(_light.rightTransformed(), _light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightUpInstance(VertexInstanceRelative(_light.upTransformed(), _light.targetTransformed(), SelectedChangedComponentCaller(*this))),
	_lightStartInstance(VertexInstance(_light.startTransformed(), SelectedChangedComponentCaller(*this))),
	_lightEndInstance(VertexInstance(_light.endTransformed(), SelectedChangedComponentCaller(*this))),
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

void LightInstance::renderInactiveComponents(Renderer& renderer, const VolumeTest& volume, const bool selected) const {
	// greebo: We are not in component selection mode (and the light is still selected), 
	// check if we should draw the center of the light anyway
	if (selected && GlobalSelectionSystem().ComponentMode() != SelectionSystem::eVertex) {
		if (_light.isProjected()) {
			
		} 
		else {
			// This is a point light, check if we should always draw the light center when selected
			if (GlobalRegistry().get("user/ui/alwaysShowLightCenter") == "1") {
				_light.getDoom3Radius().setCenterColour(GlobalRadiant().getColour("light_vertex_normal"));
				_light.renderLightCentre(renderer, volume, Instance::localToWorld());
			}
		}
	}
}

/* greebo: This is the method that gets called by renderer.h. It passes the call on to the Light class render methods. 
 */
void LightInstance::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	const bool lightIsSelected = getSelectable().isSelected();
	_light.renderSolid(renderer, volume, Instance::localToWorld(), lightIsSelected);
	
	renderInactiveComponents(renderer, volume, lightIsSelected);
}
  
void LightInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	const bool lightIsSelected = getSelectable().isSelected();
	_light.renderWireframe(renderer, volume, Instance::localToWorld(), lightIsSelected);
	
	renderInactiveComponents(renderer, volume, lightIsSelected);
}

// Renders the components of this light instance 
void LightInstance::renderComponents(Renderer& renderer, const VolumeTest& volume) const {
	
	// Render the components (light center) as selected/deselected, if we are in the according mode
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex) {
		if (_light.isProjected()) {
			// A projected light
			
			// Update the colour of the light center dot
			_light.colourLightTarget() = (_lightTargetInstance.isSelected()) 
				? GlobalRadiant().getColour("light_vertex_selected") 
				: GlobalRadiant().getColour("light_vertex_deselected");

			_light.colourLightRight() = (_lightRightInstance.isSelected()) 
				? GlobalRadiant().getColour("light_vertex_selected") 
				: GlobalRadiant().getColour("light_vertex_deselected");
				
			_light.colourLightUp() = (_lightUpInstance.isSelected()) 
				? GlobalRadiant().getColour("light_vertex_selected") 
				: GlobalRadiant().getColour("light_vertex_deselected");
				
			_light.colourLightStart() = (_lightStartInstance.isSelected()) 
				? GlobalRadiant().getColour("light_startend_selected") 
				: GlobalRadiant().getColour("light_startend_deselected");
				
			_light.colourLightEnd() = (_lightEndInstance.isSelected()) 
				? GlobalRadiant().getColour("light_startend_selected") 
				: GlobalRadiant().getColour("light_startend_deselected");
			
			// Render the projection points
			_light.renderProjectionPoints(renderer, volume, Instance::localToWorld());
		}
		else {
			// A point light
			
			// Update the colour of the light center dot 
			if (_lightCenterInstance.isSelected()) {
				_light.getDoom3Radius().setCenterColour(GlobalRadiant().getColour("light_vertex_selected"));
				_light.renderLightCentre(renderer, volume, Instance::localToWorld());
			}
			else {
				_light.getDoom3Radius().setCenterColour(GlobalRadiant().getColour("light_vertex_deselected"));
				_light.renderLightCentre(renderer, volume, Instance::localToWorld());
			}
		}
	}
}

/* greebo: This returns the AABB of all the selectable vertices. This method
 * distinguishes between projected and point lights and stretches the AABB accordingly.
 */
const AABB& LightInstance::getSelectedComponentsBounds() const {
	// Create a new axis aligned bounding box
	m_aabb_component = AABB();

	if (isProjected()) {
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
	return (m_dragPlanes.isSelected() || _lightCenterInstance.isSelected()
			|| _lightTargetInstance.isSelected() || _lightRightInstance.isSelected());
}

// greebo: Selects/deselects all components, depending on the chosen componentmode
void LightInstance::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) {
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

void LightInstance::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
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
		if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex)  
		    //&& getTranslation() != Vector3(0,0,0)) 
		{
			if (_lightCenterInstance.isSelected()) {
				// Retrieve the translation and apply it to the temporary light center variable
				// This adds the translation vector to the previous light origin 
				_light.getDoom3Radius().m_centerTransformed = 
										_light.getDoom3Radius().m_center + getTranslation();
			}
			
			if (_lightTargetInstance.isSelected()) {
				// greebo: Todo: move this into the Light class
				Vector3 oldTarget = _light.target();
				Vector3 newTarget = oldTarget + getTranslation();
				
				double angle = oldTarget.angle(newTarget);
				
				// If we are at rougly 0 or 180 degrees, don't rotate anything, this is probably a translation only
				if (std::abs(angle) > 0.01 && std::abs(c_pi-angle) > 0.01) {
					// Calculate the transformation matrix defined by the two vectors
					Matrix4 rotationMatrix = Matrix4::getRotation(oldTarget, newTarget);
					_light.rightTransformed() = rotationMatrix.transform(_light.right()).getProjected();
					_light.upTransformed() = rotationMatrix.transform(_light.up()).getProjected();
					
					if (_light.useStartEnd()) {
						_light.startTransformed() = rotationMatrix.transform(_light.start()).getProjected();
						_light.endTransformed() = rotationMatrix.transform(_light.end()).getProjected();
						
						vector3_snap(_light.startTransformed(), GlobalRadiant().getGridSize());
						vector3_snap(_light.endTransformed(), GlobalRadiant().getGridSize());
					}
					
					// Snap the rotated vectors to the grid
					vector3_snap(_light.rightTransformed(), GlobalRadiant().getGridSize());
					vector3_snap(_light.upTransformed(), GlobalRadiant().getGridSize());
				}
				
				// if we are at 180 degrees, invert the light_start and light_end vectors
				if (std::abs(c_pi-angle) < 0.01) {
					if (_light.useStartEnd()) {
						_light.startTransformed() = -_light.start();
						_light.endTransformed() = -_light.end();
					}

					_light.rightTransformed() = -_light.right();
					_light.upTransformed() = -_light.up();
				}
				
				// Save the new target
				_light.targetTransformed() = newTarget;
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
				// Save the new light_up vector
				_light.startTransformed() = _light.start() + getTranslation();
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
