#include "Light.h"

#include "iradiant.h"
#include "igrid.h"
#include "Doom3LightRadius.h"
#include "LightShader.h"
#include "LightSettings.h"

#include "LightNode.h"

namespace entity {

// Initialise the static default shader string
std::string LightShader::m_defaultShader = "";

// ------ Helper Functions ----------------------------------------------------------

/* greebo: Calculates the eight vertices defining the light corners as defined by the passed AABB. 
 */
void light_vertices(const AABB& aabb_light, Vector3 points[6]) {
  Vector3 max(aabb_light.origin + aabb_light.extents);
  Vector3 min(aabb_light.origin - aabb_light.extents);
  Vector3 mid(aabb_light.origin);

  // top, bottom, tleft, tright, bright, bleft
  points[0] = Vector3(mid[0], mid[1], max[2]);
  points[1] = Vector3(mid[0], mid[1], min[2]);
  points[2] = Vector3(min[0], max[1], mid[2]);
  points[3] = Vector3(max[0], max[1], mid[2]);
  points[4] = Vector3(max[0], min[1], mid[2]);
  points[5] = Vector3(min[0], min[1], mid[2]);
}

/* greebo: light_draw() gets called by the render() function of the Light class.
 * It basically draws the small diamond representing the light origin 
 */
void light_draw(const AABB& aabb_light, RenderStateFlags state) {
  Vector3 points[6];
  
  // Revert the light "diamond" to default extents for drawing
  AABB tempAABB;
  tempAABB.origin = aabb_light.origin;
  tempAABB.extents = Vector3(8,8,8);
   
   // Calculate the light vertices of this bounding box and store them into <points>
  light_vertices(tempAABB, points);

  	// greebo: Draw the small cube representing the light origin.
    typedef unsigned int index_t;
    const index_t indices[24] = {
      0, 2, 3,
      0, 3, 4,
      0, 4, 5,
      0, 5, 2,
      1, 2, 5,
      1, 5, 4,
      1, 4, 3,
      1, 3, 2
    };
    glVertexPointer(3, GL_DOUBLE, 0, points);
    glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(index_t), RenderIndexTypeID, indices);
}

// ----- Light Class Implementation -------------------------------------------------

// Constructor
Light::Light(IEntityClassPtr eclass, LightNode& node, const Callback& transformChanged, const Callback& boundsChanged, const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_rotationKey(RotationChangedCaller(*this)),
	m_colour(Callback()),
	m_named(m_entity),
	m_radii_box(m_aabb_light.origin),
	_rCentre(m_doom3Radius.m_centerTransformed, m_aabb_light.origin, m_doom3Radius._centerColour),
	_rTarget(_lightTargetTransformed, m_aabb_light.origin, _colourLightTarget),
	_rUp(_lightUpTransformed, _lightTargetTransformed, m_aabb_light.origin, _colourLightUp),
	_rRight(_lightRightTransformed, _lightTargetTransformed, m_aabb_light.origin, _colourLightRight),
	_rStart(_lightStartTransformed, m_aabb_light.origin, _colourLightStart),
	_rEnd(_lightEndTransformed, m_aabb_light.origin, _colourLightEnd),
	m_renderName(m_named, m_aabb_light.origin),
	m_useLightOrigin(false),
	m_useLightRotation(false),
	m_renderProjection(m_aabb_light.origin, _lightStartTransformed, m_doom3Frustum),
	m_transformChanged(transformChanged),
	m_boundsChanged(boundsChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

// Copy Constructor
Light::Light(const Light& other, LightNode& node, const Callback& transformChanged, const Callback& boundsChanged, const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_rotationKey(RotationChangedCaller(*this)),
	m_colour(Callback()),
	m_named(m_entity),
	m_radii_box(m_aabb_light.origin),
	_rCentre(m_doom3Radius.m_centerTransformed, m_aabb_light.origin, m_doom3Radius._centerColour),
	_rTarget(_lightTargetTransformed, m_aabb_light.origin, _colourLightTarget),
	_rUp(_lightUpTransformed, _lightTargetTransformed, m_aabb_light.origin, _colourLightUp),
	_rRight(_lightRightTransformed, _lightTargetTransformed, m_aabb_light.origin, _colourLightRight),
	_rStart(_lightStartTransformed, m_aabb_light.origin, _colourLightStart),
	_rEnd(_lightEndTransformed, m_aabb_light.origin, _colourLightEnd),
	m_renderName(m_named, m_aabb_light.origin),
	m_useLightOrigin(false),
	m_useLightRotation(false),
	m_renderProjection(m_aabb_light.origin, _lightStartTransformed, m_doom3Frustum),
	m_transformChanged(transformChanged),
	m_boundsChanged(boundsChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

/* greebo: This sets up the keyObservers so that the according classes get notified when any
 * of the key/values are changed. 
 * Note, that the entity key/values are still empty at the point where this method is called.
 */
void Light::construct() {
	_colourLightTarget = Vector3(255,255,0);
	_colourLightUp = Vector3(255,0,255);
	_colourLightRight = Vector3(255,0,255);
	_colourLightStart = Vector3(0,0,0);
	_colourLightEnd = Vector3(0,0,0);
	
	default_rotation(m_rotation);
	m_aabb_light.origin = Vector3(0, 0, 0);
	default_extents(m_aabb_light.extents);

	m_keyObservers.insert("name", NamedEntity::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("_color", Colour::ColourChangedCaller(m_colour));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));

	m_keyObservers.insert("angle", RotationKey::AngleChangedCaller(m_rotationKey));
	m_keyObservers.insert("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	m_keyObservers.insert("light_radius", Doom3LightRadius::LightRadiusChangedCaller(m_doom3Radius));
	m_keyObservers.insert("light_center", Doom3LightRadius::LightCenterChangedCaller(m_doom3Radius));
	m_keyObservers.insert("light_origin", Light::LightOriginChangedCaller(*this));
	m_keyObservers.insert("light_rotation", Light::LightRotationChangedCaller(*this));
	m_keyObservers.insert("light_target", Light::LightTargetChangedCaller(*this));
	m_keyObservers.insert("light_up", Light::LightUpChangedCaller(*this));
	m_keyObservers.insert("light_right", Light::LightRightChangedCaller(*this));
	m_keyObservers.insert("light_start", Light::LightStartChangedCaller(*this));
	m_keyObservers.insert("light_end", Light::LightEndChangedCaller(*this));
	m_keyObservers.insert("texture", LightShader::ValueChangedCaller(m_shader));
	m_useLightTarget = m_useLightUp = m_useLightRight = m_useLightStart = m_useLightEnd = false;
	m_doom3ProjectionChanged = true;

	// set the colours to their default values
	m_doom3Radius.setCenterColour(m_entity.getEntityClass()->getColour());

	m_entity.setIsContainer(true);
}

void Light::updateOrigin() {
	m_boundsChanged();

	m_doom3Radius.m_changed();

    // Update the projection as well, if necessary
    if (isProjected())
        projectionChanged();

	GlobalSelectionSystem().pivotChanged();
}

void Light::originChanged() {
	m_aabb_light.origin = m_useLightOrigin ? m_lightOrigin : m_originKey.m_origin;
	updateOrigin();
}

void Light::lightOriginChanged(const std::string& value) {
	m_useLightOrigin = (!value.empty());
	if (m_useLightOrigin) {
		read_origin(m_lightOrigin, value);
	}
	originChanged();
}

void Light::lightTargetChanged(const std::string& value) {
	m_useLightTarget = (!value.empty());
	if (m_useLightTarget) {
		read_origin(_lightTarget, value);
	}
	_lightTargetTransformed = _lightTarget;
	projectionChanged();
}

void Light::lightUpChanged(const std::string& value) {
	m_useLightUp = (!value.empty());
	if (m_useLightUp) {
		read_origin(_lightUp, value);
	}
	_lightUpTransformed = _lightUp;
	projectionChanged();
}

void Light::lightRightChanged(const std::string& value) {
	m_useLightRight = (!value.empty());
	if (m_useLightRight) {
		read_origin(_lightRight, value);
	}
	_lightRightTransformed = _lightRight;
	projectionChanged();
}

void Light::lightStartChanged(const std::string& value) {
	m_useLightStart = (!value.empty());
	if (m_useLightStart) {
		read_origin(_lightStart, value);
	}
	_lightStartTransformed = _lightStart;
	
	// If the light_end key is still unused, set it to a reasonable value
	if (m_useLightEnd) {
		checkStartEnd();
	}
	
	projectionChanged();
}

void Light::lightEndChanged(const std::string& value) {
	m_useLightEnd = (!value.empty());
	if (m_useLightEnd) {
		read_origin(_lightEnd, value);
	}
	
	_lightEndTransformed = _lightEnd;
	
	// If the light_start key is still unused, set it to a reasonable value
	if (m_useLightStart) {
		checkStartEnd();
	}
	
	projectionChanged();
}

/* greebo: Checks the light_start and light_end keyvals for meaningful values.
 *
 * If the light_end is "above" the light_start (i.e. nearer to the origin),
 * the two are swapped.
 * 
 * This also checks if the two vertices happen to be on the very same spot. 
 */
void Light::checkStartEnd() {
	if (m_useLightStart && m_useLightEnd) {
		if (_lightEnd.getLengthSquared() < _lightStart.getLengthSquared()) {
			// Swap the two vectors
			Vector3 temp;
			temp = _lightEnd;
			_lightEndTransformed = _lightEnd = _lightStart;
			_lightStartTransformed = _lightStart = temp;
		}
		
		// The light_end on the same point as the light_start is an unlucky situation, revert it
		// otherwise the vertices won't be separable again for the user 
		if (_lightEnd == _lightStart) {
			_lightEndTransformed = _lightEnd = _lightTarget;  
			_lightStartTransformed = _lightStart = Vector3(0,0,0);
		}
	}
}

void Light::writeLightOrigin() {
	write_origin(m_lightOrigin, &m_entity, "light_origin");
}

void Light::rotationChanged() {
	rotation_assign(m_rotation, m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation);
	GlobalSelectionSystem().pivotChanged();
}

void Light::lightRotationChanged(const std::string& value) {
	m_useLightRotation = (!value.empty());
	if(m_useLightRotation) {
		read_rotation(m_lightRotation, value);
	}
	rotationChanged();
}

/* greebo: Calculates the corners of the light radii box and rotates them according the rotation matrix.
 */
void Light::updateLightRadiiBox() const {
	// Get the rotation matrix
	const Matrix4& rotation = rotation_toMatrix(m_rotation);
	
	// Calculate the corners of the light radius box and store them into <m_radii_box.m_points>
	// For the first calculation an AABB with origin 0,0,0 is needed, the vectors get added 
	// to the origin AFTER they are transformed by the rotation matrix
	aabb_corners(AABB(Vector3(0, 0, 0), m_doom3Radius.m_radiusTransformed), m_radii_box.m_points);
	
	// Transform each point with the given rotation matrix and add the vectors to the light origin 
	matrix4_transform_point(rotation, m_radii_box.m_points[0]);
	m_radii_box.m_points[0] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[1]);
	m_radii_box.m_points[1] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[2]);
	m_radii_box.m_points[2] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[3]);
	m_radii_box.m_points[3] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[4]);
	m_radii_box.m_points[4] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[5]);
	m_radii_box.m_points[5] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[6]);
	m_radii_box.m_points[6] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[7]);
	m_radii_box.m_points[7] += m_aabb_light.origin;
}

void Light::instanceAttach(const scene::Path& path) {
	if(++m_instanceCounter.m_count == 1) {
		m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		m_entity.attach(m_keyObservers);
	}
}

void Light::instanceDetach(const scene::Path& path) {
	if(--m_instanceCounter.m_count == 0) {
		m_entity.detach(m_keyObservers);
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

/* greebo: Snaps the current light origin to the grid. 
 * 
 * Note: This gets called when the light as a whole is selected, NOT in vertex editing mode
 */
void Light::snapto(float snap) {
	if (m_useLightOrigin) {
		m_lightOrigin = origin_snapped(m_lightOrigin, snap);
		writeLightOrigin();
	}
	else {
		m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
		m_originKey.write(&m_entity);
	}
}

void Light::setLightRadius(const AABB& aabb) {
	m_aabb_light.origin = aabb.origin;
	m_doom3Radius.m_radiusTransformed = aabb.extents;
}

void Light::transformLightRadius(const Matrix4& transform) {
	matrix4_transform_point(transform, m_aabb_light.origin);
}

void Light::revertTransform() {
	m_aabb_light.origin = m_useLightOrigin ? m_lightOrigin : m_originKey.m_origin;
	rotation_assign(m_rotation, m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation);
	m_doom3Radius.m_radiusTransformed = m_doom3Radius.m_radius;
	m_doom3Radius.m_centerTransformed = m_doom3Radius.m_center;
	
	// revert all the projection changes to the saved values
	_lightTargetTransformed = _lightTarget;
	_lightRightTransformed = _lightRight;
	_lightUpTransformed = _lightUp;
	_lightStartTransformed = _lightStart;
	_lightEndTransformed = _lightEnd;
}

void Light::freezeTransform() {
	if (m_useLightOrigin) {
		m_lightOrigin = m_aabb_light.origin;
		writeLightOrigin();
	}
	else {
		m_originKey.m_origin = m_aabb_light.origin;
		m_originKey.write(&m_entity);
	}
    
    if (isProjected()) {
	    if (m_useLightTarget) {
			_lightTarget = _lightTargetTransformed;
			m_entity.setKeyValue("light_target", _lightTarget);
		}
		
		if (m_useLightUp) {
			_lightUp = _lightUpTransformed;
			m_entity.setKeyValue("light_up", _lightUp);
		}
		
		if (m_useLightRight) {
			_lightRight = _lightRightTransformed;
			m_entity.setKeyValue("light_right", _lightRight);
		}
		
		// Check the start and end (if the end is "above" the start, for example)
		checkStartEnd();
		
		if (m_useLightStart) {
			_lightStart = _lightStartTransformed;
			m_entity.setKeyValue("light_start", _lightStart);
		}
		
		if (m_useLightEnd) {
			_lightEnd = _lightEndTransformed;
			m_entity.setKeyValue("light_end", _lightEnd);
		}		
    }
    else {
    	// Save the light center to the entity key/values
		m_doom3Radius.m_center = m_doom3Radius.m_centerTransformed;
		m_entity.setKeyValue("light_center", m_doom3Radius.m_center);
    }
	
	if(m_useLightRotation) {
		rotation_assign(m_lightRotation, m_rotation);
		write_rotation(m_lightRotation, &m_entity, "light_rotation");
	}

	rotation_assign(m_rotationKey.m_rotation, m_rotation);
	write_rotation(m_rotationKey.m_rotation, &m_entity);

	if (!isProjected()) {
		m_doom3Radius.m_radius = m_doom3Radius.m_radiusTransformed;
		write_origin(m_doom3Radius.m_radius, &m_entity, "light_radius");
	}
}

entity::Doom3Entity& Light::getEntity() {
	return m_entity;
}
const entity::Doom3Entity& Light::getEntity() const {
	return m_entity;
}

/*Namespaced& Light::getNamespaced() {
	return m_nameKeys;
}*/
const NamedEntity& Light::getNameable() const {
	return m_named;
}
NamedEntity& Light::getNameable() {
	return m_named;
}
TransformNode& Light::getTransformNode() {
	return m_transform;
}
const TransformNode& Light::getTransformNode() const {
	return m_transform;
}

// Backend render function (GL calls)
void Light::render(RenderStateFlags state) const {
	light_draw(m_aabb_light, state);
}

VolumeIntersectionValue Light::intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const {
	return volume.TestAABB(m_aabb_light, localToWorld);
}

Doom3LightRadius& Light::getDoom3Radius() {
	return m_doom3Radius;
}

void Light::renderProjectionPoints(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
	// Add the renderable light target
	renderer.Highlight(Renderer::ePrimitive, false);
	renderer.Highlight(Renderer::eFace, false);
	
	renderer.SetState(_rRight.getShader(), Renderer::eFullMaterials);
	renderer.SetState(_rRight.getShader(), Renderer::eWireframeOnly);
	renderer.addRenderable(_rRight, localToWorld);
	
	renderer.SetState(_rUp.getShader(), Renderer::eFullMaterials);
	renderer.SetState(_rUp.getShader(), Renderer::eWireframeOnly);
	renderer.addRenderable(_rUp, localToWorld);
	
	renderer.SetState(_rTarget.getShader(), Renderer::eFullMaterials);
	renderer.SetState(_rTarget.getShader(), Renderer::eWireframeOnly);
	renderer.addRenderable(_rTarget, localToWorld);
	
	if (m_useLightStart) {
		renderer.SetState(_rStart.getShader(), Renderer::eFullMaterials);
		renderer.SetState(_rStart.getShader(), Renderer::eWireframeOnly);
		renderer.addRenderable(_rStart, localToWorld);
	}
	
	if (m_useLightEnd) {
		renderer.SetState(_rEnd.getShader(), Renderer::eFullMaterials);
		renderer.SetState(_rEnd.getShader(), Renderer::eWireframeOnly);
		renderer.addRenderable(_rEnd, localToWorld);
	}
}

// Adds the light centre renderable to the given renderer
void Light::renderLightCentre(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
	renderer.Highlight(Renderer::ePrimitive, false);
	renderer.Highlight(Renderer::eFace, false);
	renderer.SetState(_rCentre.getShader(), Renderer::eFullMaterials);
	renderer.SetState(_rCentre.getShader(), Renderer::eWireframeOnly);
      
	renderer.addRenderable(_rCentre, localToWorld);
}

void Light::renderWireframe(Renderer& renderer, 
							const VolumeTest& volume, 
							const Matrix4& localToWorld, 
							bool selected) const 
{
	// Main render, submit the diamond that represents the light entity
	renderer.SetState(
		m_entity.getEntityClass()->getWireShader(), Renderer::eWireframeOnly
	);
	renderer.SetState(
		m_entity.getEntityClass()->getWireShader(), Renderer::eFullMaterials
	);
	renderer.addRenderable(*this, localToWorld);

	// Render bounding box if selected or the showAllLighRadii flag is set
	if (selected || LightSettings().showAllLightRadii()) {

		if (isProjected()) {
			// greebo: This is not much of an performance impact as the projection gets only recalculated when it has actually changed.
			projection();
			renderer.addRenderable(m_renderProjection, localToWorld);
		}
		else {
			updateLightRadiiBox();
			renderer.addRenderable(m_radii_box, localToWorld);
		}
	}

	// Render the name
	if (isNameVisible()) {
		renderer.addRenderable(m_renderName, localToWorld);
	}
}

void Light::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld) {
	test.BeginMesh(localToWorld);

	SelectionIntersection best;
	aabb_testselect(m_aabb_light, test, best);
	if (best.valid()) {
		selector.addIntersection(best);
	}
}

void Light::translate(const Vector3& translation) {
	m_aabb_light.origin = origin_translated(m_aabb_light.origin, translation);
}

/* greebo: This translates the light start with the given <translation>
 * Checks, if the light_start is positioned "above" the light origin and constrains
 * the movement accordingly to prevent the light volume to become an "hourglass".
 */
void Light::translateLightStart(const Vector3& translation) {
	Vector3 candidate = _lightStart + translation;
	
	Vector3 assumedEnd = (m_useLightEnd) ? _lightEndTransformed : _lightTargetTransformed;
	
	Vector3 normal = (candidate - assumedEnd).getNormalised();
	
	// Calculate the distance to the plane going through the origin, hence the minus sign
	double dist = normal.dot(candidate);
	
	if (dist > 0) {
		// Light_Start is too "high", project it back onto the origin plane 
		_lightStartTransformed = candidate - normal*dist;
		vector3_snap(_lightStartTransformed, GlobalGrid().getGridSize());
	}
	else {
		// The candidate seems to be ok, apply it to the selection
		_lightStartTransformed = candidate;
	}
}

void Light::translateLightTarget(const Vector3& translation) {
	Vector3 oldTarget = _lightTarget;
	Vector3 newTarget = oldTarget + translation;
	
	double angle = oldTarget.angle(newTarget);
	
	// If we are at roughly 0 or 180 degrees, don't rotate anything, this is probably a pure translation
	if (std::abs(angle) > 0.01 && std::abs(c_pi-angle) > 0.01) {
		// Calculate the transformation matrix defined by the two vectors
		Matrix4 rotationMatrix = Matrix4::getRotation(oldTarget, newTarget);
		_lightRightTransformed = rotationMatrix.transform(_lightRight).getProjected();
		_lightUpTransformed = rotationMatrix.transform(_lightUp).getProjected();
		
		if (m_useLightStart && m_useLightEnd) {
			_lightStartTransformed = rotationMatrix.transform(_lightStart).getProjected();
			_lightEndTransformed = rotationMatrix.transform(_lightEnd).getProjected();
			
			vector3_snap(_lightStartTransformed, GlobalGrid().getGridSize());
			vector3_snap(_lightEndTransformed, GlobalGrid().getGridSize());
		}
		
		// Snap the rotated vectors to the grid
		vector3_snap(_lightRightTransformed, GlobalGrid().getGridSize());
		vector3_snap(_lightUpTransformed, GlobalGrid().getGridSize());
	}
	
	// if we are at 180 degrees, invert the light_start and light_end vectors
	if (std::abs(c_pi-angle) < 0.01) {
		if (m_useLightStart && m_useLightEnd) {
			_lightStartTransformed = -_lightStart;
			_lightEndTransformed = -_lightEnd;
		}

		_lightRightTransformed = -_lightRight;
		_lightUpTransformed = -_lightUp;
	}
	
	// Save the new target
	_lightTargetTransformed = newTarget;
}

void Light::rotate(const Quaternion& rotation) {
	if (isProjected()) {
		// Retrieve the rotation matrix...
		Matrix4 rotationMatrix = matrix4_rotation_for_quaternion(rotation);
		
		// ... and apply it to all the vertices defining the projection
		_lightTargetTransformed = rotationMatrix.transform(_lightTarget).getProjected();
		_lightRightTransformed = rotationMatrix.transform(_lightRight).getProjected();
		_lightUpTransformed = rotationMatrix.transform(_lightUp).getProjected();
		_lightStartTransformed = rotationMatrix.transform(_lightStart).getProjected();
		_lightEndTransformed = rotationMatrix.transform(_lightEnd).getProjected();
	}
	else {
		rotation_rotate(m_rotation, rotation);
	}
}

void Light::transformChanged() {
	revertTransform();
	m_evaluateTransform();
	updateOrigin();
}

const Matrix4& Light::getLocalPivot() const {
	m_localPivot = rotation_toMatrix(m_rotation);
	m_localPivot.t().getVector3() = m_aabb_light.origin;
	return m_localPivot;
}

void Light::setLightChangedCallback(const Callback& callback) {
	m_doom3Radius.m_changed = callback;
}

// greebo: This returns the AABB of the WHOLE light (this includes the volume and all its selectable vertices)
// Used to test the light for selection on mouse click.
const AABB& Light::aabb() const {
	if (isProjected()) {
		// start with an empty AABB and include all the projection vertices
		m_doom3AABB = AABB();
		m_doom3AABB.includePoint(m_aabb_light.origin);
		m_doom3AABB.includePoint(m_aabb_light.origin + _lightTargetTransformed);
		m_doom3AABB.includePoint(m_aabb_light.origin + _lightTargetTransformed + _lightRightTransformed);
		m_doom3AABB.includePoint(m_aabb_light.origin + _lightTargetTransformed + _lightUpTransformed);
		if (useStartEnd()) {
			m_doom3AABB.includePoint(m_aabb_light.origin + _lightStartTransformed);
			m_doom3AABB.includePoint(m_aabb_light.origin + _lightEndTransformed);
		}
	}
	else {
		m_doom3AABB = AABB(m_aabb_light.origin, m_doom3Radius.m_radiusTransformed);
		// greebo: Make sure the light center (that maybe outside of the light volume) is selectable
		m_doom3AABB.includePoint(m_aabb_light.origin + m_doom3Radius.m_centerTransformed);		
	}
	return m_doom3AABB;
}

const AABB& Light::localAABB() const {
  	return aabb();
}

/* This is needed for the drag manipulator to check the aabb of the light volume only (excl. the light center)
 */
const AABB& Light::lightAABB() const {
	_lightAABB = AABB(m_aabb_light.origin, m_doom3Radius.m_radiusTransformed);
  	return _lightAABB;
}
  
bool Light::testAABB(const AABB& other) const {
	if (isProjected()) {
		Matrix4 transform = rotation();
		transform.t().getVector3() = localAABB().origin;
		projection();
		Frustum frustum(frustum_transformed(m_doom3Frustum, transform));
		return frustum_test_aabb(frustum, other) != c_volumeOutside;
    }
    
	// test against an AABB which contains the rotated bounds of this light.
	const AABB& bounds = aabb();
	return aabb_intersects_aabb(other, AABB(
		bounds.origin,
		Vector3(
			static_cast<float>(fabs(m_rotation[0] * bounds.extents[0])
								+ fabs(m_rotation[3] * bounds.extents[1])
								+ fabs(m_rotation[6] * bounds.extents[2])),
			static_cast<float>(fabs(m_rotation[1] * bounds.extents[0])
								+ fabs(m_rotation[4] * bounds.extents[1])
								+ fabs(m_rotation[7] * bounds.extents[2])),
			static_cast<float>(fabs(m_rotation[2] * bounds.extents[0])
								+ fabs(m_rotation[5] * bounds.extents[1])
								+ fabs(m_rotation[8] * bounds.extents[2]))
		)
	));
}

const Matrix4& Light::rotation() const {
	m_doom3Rotation = rotation_toMatrix(m_rotation);
	return m_doom3Rotation;
}

/* greebo: This is needed by the renderer to determine the center of the light. It returns
 * the centerTransformed variable as the lighting should be updated as soon as the light center
 * is dragged.
 * 
 * Note: In order to render projected lights correctly, I made the projection render code to use
 * this method to determine the center of projection, hence the if (isProjected()) clause
 */
const Vector3& Light::offset() const {
	if (isProjected()) 
    {
		return _projectionCenter;
	}
	else 
    {
		return m_doom3Radius.m_centerTransformed;
	}
}
const Vector3& Light::colour() const {
	return m_colour.m_colour;
}

Vector3& Light::target() 			{ return _lightTarget; }
Vector3& Light::targetTransformed() { return _lightTargetTransformed; }
Vector3& Light::up() 				{ return _lightUp; }
Vector3& Light::upTransformed() 	{ return _lightUpTransformed; }
Vector3& Light::right() 			{ return _lightRight; }
Vector3& Light::rightTransformed() 	{ return _lightRightTransformed; }
Vector3& Light::start() 			{ return _lightStart; }
Vector3& Light::startTransformed() 	{ return _lightStartTransformed; }
Vector3& Light::end() 				{ return _lightEnd; }
Vector3& Light::endTransformed() 	{ return _lightEndTransformed; }

Vector3& Light::colourLightTarget()	{ return _colourLightTarget; }
Vector3& Light::colourLightRight() 	{ return _colourLightRight; }
Vector3& Light::colourLightUp() 	{ return _colourLightUp; }
Vector3& Light::colourLightStart()	{ return _colourLightStart; }
Vector3& Light::colourLightEnd()	{ return _colourLightEnd; }

/* greebo: A light is projected, if the entity keys light_target/light_up/light_right are not empty.
 */
bool Light::isProjected() const {
	return m_useLightTarget && m_useLightUp && m_useLightRight;
}

// greebo: Returns true if BOTH the light_start and light_end vectors are used
bool Light::useStartEnd() const {
	return m_useLightStart && m_useLightEnd;
}

void Light::projectionChanged() 
{
	m_doom3ProjectionChanged = true;
	m_doom3Radius.m_changed();

    // Calculate the projection centre
	_projectionCenter = m_aabb_light.origin + _lightTargetTransformed;

	SceneChangeNotify();
}

const Matrix4& Light::projection() const {
	if (!m_doom3ProjectionChanged) {
		return m_doom3Projection;
	}
	
	m_doom3ProjectionChanged = false;
	m_doom3Projection = g_matrix4_identity;
	matrix4_translate_by_vec3(m_doom3Projection, Vector3(0.5f, 0.5f, 0));
	matrix4_scale_by_vec3(m_doom3Projection, Vector3(0.5f, 0.5f, 1));

	Plane3 lightProject[4];

	// If there is a light_start key set, use this, otherwise use the unit vector of the target direction  
	Vector3 start = m_useLightStart && m_useLightEnd ? _lightStartTransformed : _lightTargetTransformed.getNormalised();

	// If there is no light_end, but a light_start, assume light_end = light_target
	Vector3 stop = m_useLightStart && m_useLightEnd ? _lightEndTransformed : _lightTargetTransformed;
	
	double rLen = _lightRightTransformed.getLength();
	Vector3 right = _lightRightTransformed / rLen;
	double uLen = _lightUpTransformed.getLength();
	Vector3 up = _lightUpTransformed / uLen;
	Vector3 normal = up.crossProduct(right).getNormalised();

	double dist = _lightTargetTransformed.dot(normal);
	if ( dist < 0 ) {
		dist = -dist;
		normal = -normal;
	}

	right *= ( 0.5f * dist ) / rLen;
	up *= -( 0.5f * dist ) / uLen;

	lightProject[2] = Plane3(normal, 0);
	lightProject[0] = Plane3(right, 0);
	lightProject[1] = Plane3(up, 0);

	// now offset to center
	Vector4 targetGlobal(_lightTargetTransformed, 1);
    {
		double a = targetGlobal.dot(plane3_to_vector4(lightProject[0]));
		double b = targetGlobal.dot(plane3_to_vector4(lightProject[2]));
		double ofs = 0.5 - a / b;
		plane3_to_vector4(lightProject[0]) += plane3_to_vector4(lightProject[2]) * ofs;
	}
	{
		double a = targetGlobal.dot(plane3_to_vector4(lightProject[1]));
		double b = targetGlobal.dot(plane3_to_vector4(lightProject[2]));
		double ofs = 0.5 - a / b;
		plane3_to_vector4(lightProject[1]) += plane3_to_vector4(lightProject[2]) * ofs;
	}

	// set the falloff vector
	Vector3 falloff = stop - start;
	double length = falloff.getLength();
	falloff /= length;
	if ( length <= 0 ) {
		length = 1;
	}
	falloff *= (1.0f / length);
	lightProject[3] = Plane3(falloff, -start.dot(falloff));

	// we want the planes of s=0, s=q, t=0, and t=q
	m_doom3Frustum.left = lightProject[0];
	m_doom3Frustum.bottom = lightProject[1];
	m_doom3Frustum.right = Plane3(lightProject[2].normal() - lightProject[0].normal(), lightProject[2].dist() - lightProject[0].dist());
	m_doom3Frustum.top = Plane3(lightProject[2].normal() - lightProject[1].normal(), lightProject[2].dist() - lightProject[1].dist());

	// we want the planes of s=0 and s=1 for front and rear clipping planes
	m_doom3Frustum.front = lightProject[3];

	m_doom3Frustum.back = lightProject[3];
	m_doom3Frustum.back.dist() -= 1.0f;
	m_doom3Frustum.back = -m_doom3Frustum.back;

	Matrix4 test(matrix4_from_planes(m_doom3Frustum.left, m_doom3Frustum.right, m_doom3Frustum.bottom, m_doom3Frustum.top, m_doom3Frustum.front, m_doom3Frustum.back));
	matrix4_multiply_by_matrix4(m_doom3Projection, test);

	m_doom3Frustum.left = m_doom3Frustum.left.getNormalised();
	m_doom3Frustum.right = m_doom3Frustum.right.getNormalised();
	m_doom3Frustum.bottom = m_doom3Frustum.bottom.getNormalised();
	m_doom3Frustum.top = m_doom3Frustum.top.getNormalised();
	m_doom3Frustum.back = m_doom3Frustum.back.getNormalised();
	m_doom3Frustum.front = m_doom3Frustum.front.getNormalised();
	
	//matrix4_scale_by_vec3(m_doom3Projection, Vector3(1.0 / 128, 1.0 / 128, 1.0 / 128));
	return m_doom3Projection;
}

ShaderPtr Light::getShader() const {
	return m_shader.get();
}

} // namespace entity 
