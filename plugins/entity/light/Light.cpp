#include "Light.h"

#include "Doom3LightRadius.h"
#include "LightShader.h"

LightType g_lightType = LIGHTTYPE_DEFAULT;

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
 * It looks like there is some sort of support for the rendering of projected lights in here,
 * but it's disabled at the moment. 
 */
void light_draw(const AABB& aabb_light, RenderStateFlags state) {
  Vector3 points[6];
  
  // Revert the light "diamond" to default extents for drawing
  AABB tempAABB;
  tempAABB.origin = aabb_light.origin;
  tempAABB.extents = Vector3(8,8,8);
   
   // Calculate the light vertices of this bounding box and store them into <points>
  light_vertices(tempAABB, points);

  if(state & RENDER_LIGHTING)
  {
    const float f = 0.70710678f;
    // North, East, South, West
    const Vector3 normals[8] = {
      Vector3( 0, f, f ),
      Vector3( f, 0, f ),
      Vector3( 0,-f, f ),
      Vector3(-f, 0, f ),
      Vector3( 0, f,-f ),
      Vector3( f, 0,-f ),
      Vector3( 0,-f,-f ),
      Vector3(-f, 0,-f ),
    };

#if !defined(USE_TRIANGLE_FAN)
    glBegin(GL_TRIANGLES);
#else
    glBegin(GL_TRIANGLE_FAN);
#endif
    glVertex3fv(points[0]);
    glVertex3fv(points[2]);
    glNormal3fv(normals[0]);
    glVertex3fv(points[3]);

#if !defined(USE_TRIANGLE_FAN)
    glVertex3fv(points[0]);
    glVertex3fv(points[3]);
#endif
    glNormal3fv(normals[1]);
    glVertex3fv(points[4]);

#if !defined(USE_TRIANGLE_FAN)
    glVertex3fv(points[0]);
    glVertex3fv(points[4]);
#endif
    glNormal3fv(normals[2]);
    glVertex3fv(points[5]);
#if !defined(USE_TRIANGLE_FAN)
    glVertex3fv(points[0]);
    glVertex3fv(points[5]);
#endif
    glNormal3fv(normals[3]);
    glVertex3fv(points[2]);
#if defined(USE_TRIANGLE_FAN)
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
#endif

    glVertex3fv(points[1]);
    glVertex3fv(points[2]);
    glNormal3fv(normals[7]);
    glVertex3fv(points[5]);

#if !defined(USE_TRIANGLE_FAN)
    glVertex3fv(points[1]);
    glVertex3fv(points[5]);
#endif
    glNormal3fv(normals[6]);
    glVertex3fv(points[4]);

#if !defined(USE_TRIANGLE_FAN)
    glVertex3fv(points[1]);
    glVertex3fv(points[4]);
#endif
    glNormal3fv(normals[5]);
    glVertex3fv(points[3]);

#if !defined(USE_TRIANGLE_FAN)
    glVertex3fv(points[1]);
    glVertex3fv(points[3]);
#endif
    glNormal3fv(normals[4]);
    glVertex3fv(points[2]);

    glEnd();
  }
  else
  {
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
#if 1
    glVertexPointer(3, GL_FLOAT, 0, points);
    glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(index_t), RenderIndexTypeID, indices);
#else
    glBegin(GL_TRIANGLES);
    for(unsigned int i = 0; i < sizeof(indices)/sizeof(index_t); ++i)
    {
      glVertex3fv(points[indices[i]]);
    }
    glEnd();
#endif
  }


  // NOTE: prolly not relevant until some time..
  // check for DOOM lights
#if 0
  if (strlen(ValueForKey(e, "light_right")) > 0) {
    vec3_t vRight, vUp, vTarget, vTemp;
    GetVectorForKey (e, "light_right", vRight);
    GetVectorForKey (e, "light_up", vUp);
    GetVectorForKey (e, "light_target", vTarget);

    glColor3f(0, 1, 0);
    glBegin(GL_LINE_LOOP);
    VectorAdd(vTarget, e->origin, vTemp);
    VectorAdd(vTemp, vRight, vTemp);
    VectorAdd(vTemp, vUp, vTemp);
    glVertex3fv(e->origin);
    glVertex3fv(vTemp);
    VectorAdd(vTarget, e->origin, vTemp);
    VectorAdd(vTemp, vUp, vTemp);
    VectorSubtract(vTemp, vRight, vTemp);
    glVertex3fv(e->origin);
    glVertex3fv(vTemp);
    VectorAdd(vTarget, e->origin, vTemp);
    VectorAdd(vTemp, vRight, vTemp);
    VectorSubtract(vTemp, vUp, vTemp);
    glVertex3fv(e->origin);
    glVertex3fv(vTemp);
    VectorAdd(vTarget, e->origin, vTemp);
    VectorSubtract(vTemp, vUp, vTemp);
    VectorSubtract(vTemp, vRight, vTemp);
    glVertex3fv(e->origin);
    glVertex3fv(vTemp);
    glEnd();

  }
#endif
}

// ----- Light Class Implementation -------------------------------------------------

/* greebo: This sets up the keyObservers so that the according classes get notified when any
 * of the key/values are changed. 
 * Note, that the entity key/values are still empty at the point where this method is called.
 */
void Light::construct() {
	default_rotation(m_rotation);
	m_aabb_light.origin = Vector3(0, 0, 0);
	default_extents(m_aabb_light.extents);

	m_keyObservers.insert(Static<KeyIsName>::instance().m_nameKey, NamedEntity::IdentifierChangedCaller(m_named));
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
	m_doom3Radius.setCenterColour(m_entity.getEntityClass().getColour());

	m_traverse.attach(&m_traverseObservers);
	m_traverseObservers.attach(m_funcStaticOrigin);

	m_entity.m_isContainer = true;
}

void Light::destroy() {
	if(g_lightType == LIGHTTYPE_DOOM3) {
		m_traverseObservers.detach(m_funcStaticOrigin);
		m_traverse.detach(&m_traverseObservers);
	}
}

void Light::updateOrigin() {
	m_boundsChanged();

	if(g_lightType == LIGHTTYPE_DOOM3) {
		m_funcStaticOrigin.originChanged();
	}

	m_doom3Radius.m_changed();

	GlobalSelectionSystem().pivotChanged();
}

void Light::originChanged() {
	m_aabb_light.origin = m_useLightOrigin ? m_lightOrigin : m_originKey.m_origin;
	updateOrigin();
}

void Light::lightOriginChanged(const char* value) {
	m_useLightOrigin = (std::string(value) != "");
	if(m_useLightOrigin) {
		read_origin(m_lightOrigin, value);
	}
	originChanged();
}

void Light::lightTargetChanged(const char* value) {
	m_useLightTarget = (std::string(value) != "");
	if(m_useLightTarget) {
		read_origin(m_lightTarget, value);
	}
	projectionChanged();
}

void Light::lightUpChanged(const char* value) {
	m_useLightUp = (std::string(value) != "");
	if(m_useLightUp) {
		read_origin(m_lightUp, value);
	}
	projectionChanged();
}

void Light::lightRightChanged(const char* value) {
	m_useLightRight = (std::string(value) != "");
	if(m_useLightRight) {
		read_origin(m_lightRight, value);
	}
	projectionChanged();
}

void Light::lightStartChanged(const char* value) {
	m_useLightStart = (std::string(value) != "");
	if(m_useLightStart) {
		read_origin(m_lightStart, value);
	}
	projectionChanged();
}

void Light::lightEndChanged(const char* value) {
	m_useLightEnd = (std::string(value) != "");
	if(m_useLightEnd) {
		read_origin(m_lightEnd, value);
	}
	projectionChanged();
}

void Light::writeLightOrigin() {
	write_origin(m_lightOrigin, &m_entity, "light_origin");
}

void Light::rotationChanged() {
	rotation_assign(m_rotation, m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation);
	GlobalSelectionSystem().pivotChanged();
}

void Light::lightRotationChanged(const char* value) {
	m_useLightRotation = (std::string(value) != "");
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
		if(g_lightType == LIGHTTYPE_DOOM3) {
			m_traverse.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		}
		m_entity.attach(m_keyObservers);

		if(g_lightType == LIGHTTYPE_DOOM3) {
			m_funcStaticOrigin.enable();
		}
	}
}

void Light::instanceDetach(const scene::Path& path) {
	if(--m_instanceCounter.m_count == 0) {
		if(g_lightType == LIGHTTYPE_DOOM3) {
			m_funcStaticOrigin.disable();
		}
		
		m_entity.detach(m_keyObservers);
		
		if(g_lightType == LIGHTTYPE_DOOM3) {
			m_traverse.instanceDetach(path_find_mapfile(path.begin(), path.end()));
		}
		
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

void Light::snapto(float snap) {
	if(g_lightType == LIGHTTYPE_DOOM3 && !m_useLightOrigin && !m_traverse.empty()) {
		m_useLightOrigin = true;
		m_lightOrigin = m_originKey.m_origin;
	}

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
}

void Light::freezeTransform() {
	if (g_lightType == LIGHTTYPE_DOOM3 && !m_useLightOrigin && !m_traverse.empty()) {
		m_useLightOrigin = true;
	}

	if (m_useLightOrigin) {
		m_lightOrigin = m_aabb_light.origin;
		writeLightOrigin();
	}
	else {
		m_originKey.m_origin = m_aabb_light.origin;
		m_originKey.write(&m_entity);
	}
    
	// Save the light center to the entity key/values
	m_doom3Radius.m_center = m_doom3Radius.m_centerTransformed;
	m_entity.setKeyValue("light_center", m_doom3Radius.m_center);
    
	if (g_lightType == LIGHTTYPE_DOOM3) {
		if(!m_useLightRotation && !m_traverse.empty()) {
			m_useLightRotation = true;
		}

		if(m_useLightRotation) {
			rotation_assign(m_lightRotation, m_rotation);
			write_rotation(m_lightRotation, &m_entity, "light_rotation");
		}

		rotation_assign(m_rotationKey.m_rotation, m_rotation);
		write_rotation(m_rotationKey.m_rotation, &m_entity);

		m_doom3Radius.m_radius = m_doom3Radius.m_radiusTransformed;
		write_origin(m_doom3Radius.m_radius, &m_entity, "light_radius");
	}
}

EntityKeyValues& Light::getEntity() {
	return m_entity;
}
const EntityKeyValues& Light::getEntity() const {
	return m_entity;
}

scene::Traversable& Light::getTraversable() {
	return m_traverse;
}
Namespaced& Light::getNamespaced() {
	return m_nameKeys;
}
Nameable& Light::getNameable() {
	return m_named;
}
TransformNode& Light::getTransformNode() {
	return m_transform;
}

void Light::attach(scene::Traversable::Observer* observer) {
	m_traverseObservers.attach(*observer);
}
void Light::detach(scene::Traversable::Observer* observer) {
	m_traverseObservers.detach(*observer);
}

void Light::render(RenderStateFlags state) const {
	light_draw(m_aabb_light, state);
}

VolumeIntersectionValue Light::intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const {
	return volume.TestAABB(m_aabb_light, localToWorld);
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

Doom3LightRadius& Light::getDoom3Radius() {
	return m_doom3Radius;
}

// greebo: Note that this function has to be const according to the abstract base class definition
void Light::renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const {
	renderer.SetState(m_entity.getEntityClass().getWireShader(), Renderer::eWireframeOnly);
	renderer.SetState(m_colour.state(), Renderer::eFullMaterials);
	renderer.addRenderable(*this, localToWorld);

	renderer.SetState(m_entity.getEntityClass().getWireShader(), Renderer::eFullMaterials);

	// Always draw Doom 3 light bounding boxes, if the global is set
	if (GlobalRegistry().get("user/ui/showAllLightRadii") == "1") {
		updateLightRadiiBox();
		renderer.addRenderable(m_radii_box, localToWorld);
	}

	if (selected) {
		if (isProjected()) {
			projection();
			m_projectionOrientation = rotation();
			m_projectionOrientation.t().getVector3() = localAABB().origin;
			renderer.addRenderable(m_renderProjection, m_projectionOrientation);
		}
		else {
			updateLightRadiiBox();
			renderer.addRenderable(m_radii_box, localToWorld);
		}
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

void Light::renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const {
	renderSolid(renderer, volume, localToWorld, selected);
	if (g_showNames) {
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

void Light::rotate(const Quaternion& rotation) {
	rotation_rotate(m_rotation, rotation);
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

// greebo: This returns the AABB of the WHOLE light (this includes the volume and the light center)
// Used to test the light for selection on mouse click.
const AABB& Light::aabb() const {
	//	Use the light radius to construct the base AABB.
	m_doom3AABB = AABB(m_aabb_light.origin, m_doom3Radius.m_radiusTransformed);

	// greebo: Make sure the light center (that maybe outside of the light volume) is selectable
	m_doom3AABB.includePoint(m_aabb_light.origin + m_doom3Radius.m_centerTransformed);
	return m_doom3AABB;
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
 */
const Vector3& Light::offset() const {
	return m_doom3Radius.m_centerTransformed;
}
const Vector3& Light::colour() const {
	return m_colour.m_colour;
}

bool Light::isProjected() const {
	return m_useLightTarget && m_useLightUp && m_useLightRight;
}

void Light::projectionChanged() {
	m_doom3ProjectionChanged = true;
	m_doom3Radius.m_changed();
	SceneChangeNotify();
}

const Matrix4& Light::projection() const {
	if(!m_doom3ProjectionChanged) {
		return m_doom3Projection;
	}
	
	m_doom3ProjectionChanged = false;
	m_doom3Projection = g_matrix4_identity;
	matrix4_translate_by_vec3(m_doom3Projection, Vector3(0.5f, 0.5f, 0));
	matrix4_scale_by_vec3(m_doom3Projection, Vector3(0.5f, 0.5f, 1));

#if 0
	Vector3 right = m_lightUp.crossProduct(m_lightTarget.getNormalised());
	Vector3 up = m_lightTarget.getNormalised().crossProduct(m_lightRight);
	Vector3 target = m_lightTarget;
	Matrix4 test(
      -right.x(), -right.y(), -right.z(), 0,
      -up.x(), -up.y(), -up.z(), 0,
      -target.x(), -target.y(), -target.z(), 0,
      0, 0, 0, 1
    );
	Matrix4 frustum = matrix4_frustum(-0.01, 0.01, -0.01, 0.01, 0.01, 1.0);
	test = matrix4_full_inverse(test);
	matrix4_premultiply_by_matrix4(test, frustum);
	matrix4_multiply_by_matrix4(m_doom3Projection, test);
#elif 0
	const float nearFar = 1 / 49.5f;
	Vector3 right = m_lightUp.crossProduct((m_lightTarget + m_lightRight).getNormalised());
	Vector3 up = (m_lightTarget + m_lightUp).getNormalised().crossProduct(m_lightRight);
	Vector3 target = -(m_lightTarget * (1 + nearFar));
	float scale = -1 / m_lightTarget.getLength();
	Matrix4 test(
      -inverse(right.x()), -inverse(up.x()), -inverse(target.x()), 0,
      -inverse(right.y()), -inverse(up.y()), -inverse(target.y()), 0,
      -inverse(right.z()), -inverse(up.z()), -inverse(target.z()), scale,
      0, 0, -nearFar, 0
	);
	matrix4_multiply_by_matrix4(m_doom3Projection, test);
#elif 0
	Vector3 leftA(m_lightTarget - m_lightRight);
	Vector3 leftB(m_lightRight + m_lightUp);
	Plane3 left(leftA.crossProduct(leftB).getNormalised() * (1.0 / 128), 0);
	Vector3 rightA(m_lightTarget + m_lightRight);
	Vector3 rightB(rightA.crossProduct(m_lightTarget));
	Plane3 right(rightA.crossProduct(rightB).getNormalised() * (1.0 / 128), 0);
	Vector3 bottomA(m_lightTarget - m_lightUp);
	Vector3 bottomB(bottomA.crossProduct(m_lightTarget));
	Plane3 bottom(bottomA.crossProduct(bottomB).getNormalised() * (1.0 / 128), 0);
	Vector3 topA(m_lightTarget + m_lightUp);
	Vector3 topB(topA.crossProduct(m_lightTarget));
	Plane3 top(topA.crossProduct(topB).getNormalised() * (1.0 / 128), 0);
	Plane3 front(m_lightTarget.getNormalised() * (1.0 / 128), 1);
	Plane3 back((-m_lightTarget).getNormalised() * (1.0 / 128), 0);
	Matrix4 test(matrix4_from_planes(plane3_flipped(left), plane3_flipped(right), plane3_flipped(bottom), plane3_flipped(top), plane3_flipped(front), plane3_flipped(back)));
	matrix4_multiply_by_matrix4(m_doom3Projection, test);
#else

	Plane3 lightProject[4];

	Vector3 start = m_useLightStart && m_useLightEnd ? m_lightStart : m_lightTarget.getNormalised();
	Vector3 stop = m_useLightStart && m_useLightEnd ? m_lightEnd : m_lightTarget;

	float rLen = m_lightRight.getLength();
	Vector3 right = m_lightRight / rLen;
	float uLen = m_lightUp.getLength();
	Vector3 up = m_lightUp / uLen;
	Vector3 normal = up.crossProduct(right).getNormalised();

	float dist = m_lightTarget.dot(normal);
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
	Vector4 targetGlobal(m_lightTarget, 1);
    {
		float a = targetGlobal.dot(plane3_to_vector4(lightProject[0]));
		float b = targetGlobal.dot(plane3_to_vector4(lightProject[2]));
		float ofs = 0.5f - a / b;
		plane3_to_vector4(lightProject[0]) += plane3_to_vector4(lightProject[2]) * ofs;
	}
	{
		float a = targetGlobal.dot(plane3_to_vector4(lightProject[1]));
		float b = targetGlobal.dot(plane3_to_vector4(lightProject[2]));
		float ofs = 0.5f - a / b;
		plane3_to_vector4(lightProject[1]) += plane3_to_vector4(lightProject[2]) * ofs;
	}

	// set the falloff vector
	Vector3 falloff = stop - start;
	float length = falloff.getLength();
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
	m_doom3Frustum.back = plane3_flipped(m_doom3Frustum.back);

	Matrix4 test(matrix4_from_planes(m_doom3Frustum.left, m_doom3Frustum.right, m_doom3Frustum.bottom, m_doom3Frustum.top, m_doom3Frustum.front, m_doom3Frustum.back));
	matrix4_multiply_by_matrix4(m_doom3Projection, test);

	m_doom3Frustum.left = plane3_normalised(m_doom3Frustum.left);
	m_doom3Frustum.right = plane3_normalised(m_doom3Frustum.right);
	m_doom3Frustum.bottom = plane3_normalised(m_doom3Frustum.bottom);
	m_doom3Frustum.top = plane3_normalised(m_doom3Frustum.top);
	m_doom3Frustum.back = plane3_normalised(m_doom3Frustum.back);
	m_doom3Frustum.front = plane3_normalised(m_doom3Frustum.front);
#endif
	//matrix4_scale_by_vec3(m_doom3Projection, Vector3(1.0 / 128, 1.0 / 128, 1.0 / 128));
	return m_doom3Projection;
}

Shader* Light::getShader() const {
	return m_shader.get();
}
