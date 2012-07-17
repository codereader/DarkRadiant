#include "Light.h"

#include "iradiant.h"
#include "igrid.h"
#include "Doom3LightRadius.h"
#include "LightShader.h"
#include <boost/bind.hpp>
#include "../EntitySettings.h"

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
void light_draw(const AABB& aabb_light, RenderStateFlags state)
{
    Vector3 points[6];

    // Revert the light "diamond" to default extents for drawing
    AABB tempAABB(aabb_light.origin, Vector3(8,8,8));

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
Light::Light(Doom3Entity& entity,
             LightNode& owner,
             const Callback& transformChanged,
             const Callback& boundsChanged,
             const Callback& lightRadiusChanged)
:
    _owner(owner),
    _entity(entity),
    m_originKey(boost::bind(&Light::originChanged, this)),
    _originTransformed(ORIGINKEY_IDENTITY),
    m_rotationKey(boost::bind(&Light::rotationChanged, this)),
    _renderableRadius(_lightBox.origin),
    _renderableFrustum(_lightBox.origin, _lightStartTransformed, _frustum),
    _rCentre(m_doom3Radius.m_centerTransformed, _lightBox.origin, m_doom3Radius._centerColour),
    _rTarget(_lightTargetTransformed, _lightBox.origin, _colourLightTarget),
    _rUp(_lightUpTransformed, _lightTargetTransformed, _lightBox.origin, _colourLightUp),
    _rRight(_lightRightTransformed, _lightTargetTransformed, _lightBox.origin, _colourLightRight),
    _rStart(_lightStartTransformed, _lightBox.origin, _colourLightStart),
    _rEnd(_lightEndTransformed, _lightBox.origin, _colourLightEnd),
    m_useLightRotation(false),
    m_transformChanged(transformChanged),
    m_boundsChanged(boundsChanged)
{
    m_doom3Radius.m_changed = lightRadiusChanged;
}

// Copy Constructor
Light::Light(const Light& other,
             LightNode& owner,
             Doom3Entity& entity,
             const Callback& transformChanged,
             const Callback& boundsChanged,
             const Callback& lightRadiusChanged)
: _owner(owner),
  _entity(entity),
  m_originKey(boost::bind(&Light::originChanged, this)),
  _originTransformed(ORIGINKEY_IDENTITY),
  m_rotationKey(boost::bind(&Light::rotationChanged, this)),
  _renderableRadius(_lightBox.origin),
  _renderableFrustum(_lightBox.origin, _lightStartTransformed, _frustum),
  _rCentre(m_doom3Radius.m_centerTransformed, _lightBox.origin, m_doom3Radius._centerColour),
  _rTarget(_lightTargetTransformed, _lightBox.origin, _colourLightTarget),
  _rUp(_lightUpTransformed, _lightTargetTransformed, _lightBox.origin, _colourLightUp),
  _rRight(_lightRightTransformed, _lightTargetTransformed, _lightBox.origin, _colourLightRight),
  _rStart(_lightStartTransformed, _lightBox.origin, _colourLightStart),
  _rEnd(_lightEndTransformed, _lightBox.origin, _colourLightEnd),
  m_useLightRotation(false),
  m_transformChanged(transformChanged),
  m_boundsChanged(boundsChanged)
{
    m_doom3Radius.m_changed = lightRadiusChanged;
}

Light::~Light()
{
    destroy();
}

/* greebo: This sets up the keyObservers so that the according classes get notified when any
 * of the key/values are changed.
 * Note, that the entity key/values are still empty at the point where this method is called.
 */
void Light::construct()
{
    _colourLightTarget = Vector3(255,255,0);
    _colourLightUp = Vector3(255,0,255);
    _colourLightRight = Vector3(255,0,255);
    _colourLightStart = Vector3(0,0,0);
    _colourLightEnd = Vector3(0,0,0);

    m_rotation.setIdentity();
    _lightBox.origin = Vector3(0, 0, 0);
    _lightBox.extents = Vector3(8, 8, 8);
    _originTransformed = ORIGINKEY_IDENTITY;

    _angleObserver.setCallback(boost::bind(&RotationKey::angleChanged, &m_rotationKey, _1));
    _rotationObserver.setCallback(boost::bind(&RotationKey::rotationChanged, &m_rotationKey, _1));

    _lightRadiusObserver.setCallback(boost::bind(&Doom3LightRadius::lightRadiusChanged, &m_doom3Radius, _1));
    _lightCenterObserver.setCallback(boost::bind(&Doom3LightRadius::lightCenterChanged, &m_doom3Radius, _1));
    _lightRotationObserver.setCallback(boost::bind(&Light::lightRotationChanged, this, _1));
    _lightTargetObserver.setCallback(boost::bind(&Light::lightTargetChanged, this, _1));
    _lightUpObserver.setCallback(boost::bind(&Light::lightUpChanged, this, _1));
    _lightRightObserver.setCallback(boost::bind(&Light::lightRightChanged, this, _1));
    _lightStartObserver.setCallback(boost::bind(&Light::lightStartChanged, this, _1));
    _lightEndObserver.setCallback(boost::bind(&Light::lightEndChanged, this, _1));
    _lightTextureObserver.setCallback(boost::bind(&LightShader::valueChanged, &m_shader, _1));

    // Set the flags to their default values, before attaching the key observers,
    // which might set them to true again.
    m_useLightTarget = m_useLightUp = m_useLightRight = m_useLightStart = m_useLightEnd = false;

    _owner.addKeyObserver("origin", m_originKey);

    _owner.addKeyObserver("angle", _angleObserver);
    _owner.addKeyObserver("rotation", _rotationObserver);
    _owner.addKeyObserver("light_radius", _lightRadiusObserver);
    _owner.addKeyObserver("light_center", _lightCenterObserver);
    _owner.addKeyObserver("light_rotation", _lightRotationObserver);
    _owner.addKeyObserver("light_target", _lightTargetObserver);
    _owner.addKeyObserver("light_up", _lightUpObserver);
    _owner.addKeyObserver("light_right", _lightRightObserver);
    _owner.addKeyObserver("light_start", _lightStartObserver);
    _owner.addKeyObserver("light_end", _lightEndObserver);
    _owner.addKeyObserver("texture", _lightTextureObserver);

    m_doom3ProjectionChanged = true;

    // set the colours to their default values
    m_doom3Radius.setCenterColour(_entity.getEntityClass()->getColour());

    _entity.setIsContainer(true);

    // Load the light colour (might be inherited)
    m_shader.valueChanged(_entity.getKeyValue("texture"));
}

void Light::destroy()
{
    _owner.removeKeyObserver("origin", m_originKey);

    _owner.removeKeyObserver("angle", _angleObserver);
    _owner.removeKeyObserver("rotation", _rotationObserver);

    _owner.removeKeyObserver("light_radius", _lightRadiusObserver);
    _owner.removeKeyObserver("light_center", _lightCenterObserver);
    _owner.removeKeyObserver("light_rotation", _lightRotationObserver);
    _owner.removeKeyObserver("light_target", _lightTargetObserver);
    _owner.removeKeyObserver("light_up", _lightUpObserver);
    _owner.removeKeyObserver("light_right", _lightRightObserver);
    _owner.removeKeyObserver("light_start", _lightStartObserver);
    _owner.removeKeyObserver("light_end", _lightEndObserver);
    _owner.removeKeyObserver("texture", _lightTextureObserver);
}

void Light::updateOrigin() {
    m_boundsChanged();

    m_doom3Radius.m_changed();

    // Update the projection as well, if necessary
    if (isProjected())
        projectionChanged();

    // Update the transformation matrix
    _owner.localToParent() = Matrix4::getIdentity();
    _owner.localToParent().translateBy(worldOrigin());
    _owner.localToParent().multiplyBy(m_rotation.getMatrix4());

    // Notify all child nodes
    m_transformChanged();

    GlobalSelectionSystem().pivotChanged();
}

void Light::originChanged()
{
    // The "origin" key has been changed, reset the current working copy to that value
    _originTransformed = m_originKey.get();
    updateOrigin();
}

void Light::lightTargetChanged(const std::string& value)
{
    m_useLightTarget = (!value.empty());

    if (m_useLightTarget)
    {
        _lightTarget = string::convert<Vector3>(value);
    }

    _lightTargetTransformed = _lightTarget;
    projectionChanged();
}

void Light::lightUpChanged(const std::string& value)
{
    m_useLightUp = (!value.empty());

    if (m_useLightUp)
    {
        _lightUp = string::convert<Vector3>(value);
    }

    _lightUpTransformed = _lightUp;
    projectionChanged();
}

void Light::lightRightChanged(const std::string& value)
{
    m_useLightRight = (!value.empty());

    if (m_useLightRight)
    {
        _lightRight = string::convert<Vector3>(value);
    }

    _lightRightTransformed = _lightRight;
    projectionChanged();
}

void Light::lightStartChanged(const std::string& value) {
    m_useLightStart = (!value.empty());

    if (m_useLightStart)
    {
        _lightStart = string::convert<Vector3>(value);
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

    if (m_useLightEnd)
    {
        _lightEnd = string::convert<Vector3>(value);
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

void Light::rotationChanged()
{
    m_rotation = m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation;

    // Update the transformation matrix
    _owner.localToParent() = Matrix4::getIdentity();
    _owner.localToParent().translateBy(worldOrigin());
    _owner.localToParent().multiplyBy(m_rotation.getMatrix4());

    // Notify owner about this
    m_transformChanged();

    GlobalSelectionSystem().pivotChanged();
}

void Light::lightRotationChanged(const std::string& value) {
    m_useLightRotation = (!value.empty());
    if(m_useLightRotation) {
        m_lightRotation.readFromString(value);
    }
    rotationChanged();
}

/* greebo: Calculates the corners of the light radii box and rotates them according the rotation matrix.
 */
void Light::updateRenderableRadius() const
{
#if 0
    // Get the rotation matrix
    Matrix4 rotation = m_rotation.getMatrix4();

    // Calculate the corners of the light radius box and store them into
    // <_renderableRadius.m_points> For the first calculation an AABB with
    // origin 0,0,0 is needed, the vectors get added to the origin AFTER they
    // are transformed by the rotation matrix
    aabb_corners(
        AABB(Vector3(0, 0, 0), m_doom3Radius.m_radiusTransformed),
        _renderableRadius.m_points
    );

    // Transform each point with the given rotation matrix and add the vectors to the light origin
    matrix4_transform_point(rotation, _renderableRadius.m_points[0]);
    _renderableRadius.m_points[0] += _lightBox.origin;
    matrix4_transform_point(rotation, _renderableRadius.m_points[1]);
    _renderableRadius.m_points[1] += _lightBox.origin;
    matrix4_transform_point(rotation, _renderableRadius.m_points[2]);
    _renderableRadius.m_points[2] += _lightBox.origin;
    matrix4_transform_point(rotation, _renderableRadius.m_points[3]);
    _renderableRadius.m_points[3] += _lightBox.origin;
    matrix4_transform_point(rotation, _renderableRadius.m_points[4]);
    _renderableRadius.m_points[4] += _lightBox.origin;
    matrix4_transform_point(rotation, _renderableRadius.m_points[5]);
    _renderableRadius.m_points[5] += _lightBox.origin;
    matrix4_transform_point(rotation, _renderableRadius.m_points[6]);
    _renderableRadius.m_points[6] += _lightBox.origin;
    matrix4_transform_point(rotation, _renderableRadius.m_points[7]);
    _renderableRadius.m_points[7] += _lightBox.origin;
#endif

    // greebo: Don't rotate the light radius box, that's done via local2world
    AABB lightbox(_lightBox.origin, m_doom3Radius.m_radiusTransformed);
    lightbox.getCorners(_renderableRadius.m_points);
}

/* greebo: Snaps the current light origin to the grid.
 *
 * Note: This gets called when the light as a whole is selected, NOT in vertex editing mode
 */
void Light::snapto(float snap)
{
    m_originKey.snap(snap);
    m_originKey.write(_entity);

    _originTransformed = m_originKey.get();

    updateOrigin();
}

void Light::setLightRadius(const AABB& aabb)
{
    if (EntitySettings::InstancePtr()->dragResizeEntitiesSymmetrically())
    {
        // Leave origin unchanged, calculate the new symmetrical radius
        Vector3 delta = aabb.getExtents() - m_doom3Radius.m_radiusTransformed;

        m_doom3Radius.m_radiusTransformed += delta*2;

        // Constrain the values to barely non-zero limits (issue #1969)
        for (int i = 0; i < 3; ++i)
        {
            if (m_doom3Radius.m_radiusTransformed[i] < 0.01f)
            {
                m_doom3Radius.m_radiusTransformed[i] = 0.01f;
            }
        }
    }
    else
    {
        // Transform the origin together with the radius (pivoted transform)
        _originTransformed = aabb.origin;

        // Set the new radius
        m_doom3Radius.m_radiusTransformed = aabb.extents;
    }
}

void Light::transformLightRadius(const Matrix4& transform)
{
    _originTransformed = transform.transformPoint(_originTransformed);
}

void Light::revertTransform()
{
    _originTransformed = m_originKey.get();

    m_rotation = m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation;
    m_doom3Radius.m_radiusTransformed = m_doom3Radius.m_radius;
    m_doom3Radius.m_centerTransformed = m_doom3Radius.m_center;

    // revert all the projection changes to the saved values
    _lightTargetTransformed = _lightTarget;
    _lightRightTransformed = _lightRight;
    _lightUpTransformed = _lightUp;
    _lightStartTransformed = _lightStart;
    _lightEndTransformed = _lightEnd;
}

void Light::freezeTransform()
{
    m_originKey.set(_originTransformed);
    m_originKey.write(_entity);

    if (isProjected())
    {
        if (m_useLightTarget)
        {
            _lightTarget = _lightTargetTransformed;
            _entity.setKeyValue("light_target",
                                string::to_string(_lightTarget));
        }

        if (m_useLightUp)
        {
            _lightUp = _lightUpTransformed;
            _entity.setKeyValue("light_up",
                               string::to_string(_lightUp));
        }

        if (m_useLightRight)
        {
            _lightRight = _lightRightTransformed;
            _entity.setKeyValue("light_right",
                                string::to_string(_lightRight));
        }

        // Check the start and end (if the end is "above" the start, for example)
        checkStartEnd();

        if (m_useLightStart)
        {
            _lightStart = _lightStartTransformed;
            _entity.setKeyValue("light_start",
                                string::to_string(_lightStart));
        }

        if (m_useLightEnd)
        {
            _lightEnd = _lightEndTransformed;
            _entity.setKeyValue("light_end",
                                string::to_string(_lightEnd));
        }
    }
    else
    {
        // Save the light center to the entity key/values
        m_doom3Radius.m_center = m_doom3Radius.m_centerTransformed;
        _entity.setKeyValue("light_center",
                            string::to_string(m_doom3Radius.m_center));
    }

    if(m_useLightRotation)
    {
        m_lightRotation = m_rotation;
        m_lightRotation.writeToEntity(&_entity, "light_rotation");
    }

    m_rotationKey.m_rotation = m_rotation;
    m_rotationKey.m_rotation.writeToEntity(&_entity);

    if (!isProjected())
    {
        m_doom3Radius.m_radius = m_doom3Radius.m_radiusTransformed;

        _entity.setKeyValue("light_radius",
                            string::to_string(m_doom3Radius.m_radius));
    }
}

// Backend render function (GL calls)
void Light::render(const RenderInfo& info) const {
    light_draw(_lightBox, info.getFlags());
}

Doom3LightRadius& Light::getDoom3Radius() {
    return m_doom3Radius;
}

void Light::renderProjectionPoints(RenderableCollector& collector,
                                   const VolumeTest& volume,
                                   const Matrix4& localToWorld) const 
{
    // Add the renderable light target
    collector.highlightPrimitives(false);
    collector.highlightFaces(false);

    collector.SetState(_rRight.getShader(), RenderableCollector::eFullMaterials);
    collector.SetState(_rRight.getShader(), RenderableCollector::eWireframeOnly);
    collector.addRenderable(_rRight, localToWorld);

    collector.SetState(_rUp.getShader(), RenderableCollector::eFullMaterials);
    collector.SetState(_rUp.getShader(), RenderableCollector::eWireframeOnly);
    collector.addRenderable(_rUp, localToWorld);

    collector.SetState(_rTarget.getShader(), RenderableCollector::eFullMaterials);
    collector.SetState(_rTarget.getShader(), RenderableCollector::eWireframeOnly);
    collector.addRenderable(_rTarget, localToWorld);

    if (m_useLightStart) {
        collector.SetState(_rStart.getShader(), RenderableCollector::eFullMaterials);
        collector.SetState(_rStart.getShader(), RenderableCollector::eWireframeOnly);
        collector.addRenderable(_rStart, localToWorld);
    }

    if (m_useLightEnd) {
        collector.SetState(_rEnd.getShader(), RenderableCollector::eFullMaterials);
        collector.SetState(_rEnd.getShader(), RenderableCollector::eWireframeOnly);
        collector.addRenderable(_rEnd, localToWorld);
    }
}

// Adds the light centre renderable to the given collector
void Light::renderLightCentre(RenderableCollector& collector,
                              const VolumeTest& volume,
                              const Matrix4& localToWorld) const 
{
    collector.highlightPrimitives(false);
    collector.highlightFaces(false);
    collector.SetState(_rCentre.getShader(), RenderableCollector::eFullMaterials);
    collector.SetState(_rCentre.getShader(), RenderableCollector::eWireframeOnly);

    collector.addRenderable(_rCentre, localToWorld);
}

void Light::renderWireframe(RenderableCollector& collector,
                            const VolumeTest& volume,
                            const Matrix4& localToWorld,
                            bool selected) const
{
    // Main render, submit the diamond that represents the light entity
    collector.SetState(
        _owner.getColourShader(), RenderableCollector::eWireframeOnly
    );
    collector.SetState(
        _owner.getColourShader(), RenderableCollector::eFullMaterials
    );
    collector.addRenderable(*this, localToWorld);

    // Render bounding box if selected or the showAllLighRadii flag is set
    if (selected || EntitySettings::InstancePtr()->showAllLightRadii())
    {
        if (isProjected())
        {
            // greebo: This is not much of an performance impact as the
            // projection gets only recalculated when it has actually changed.
            projection();
            collector.addRenderable(_renderableFrustum, localToWorld);
        }
        else
        {
            updateRenderableRadius();
            collector.addRenderable(_renderableRadius, localToWorld);
        }
    }
}

void Light::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    _rCentre.setRenderSystem(renderSystem);
    _rTarget.setRenderSystem(renderSystem);
    _rUp.setRenderSystem(renderSystem);
    _rRight.setRenderSystem(renderSystem);
    _rStart.setRenderSystem(renderSystem);
    _rEnd.setRenderSystem(renderSystem);

    m_shader.setRenderSystem(renderSystem);
}

void Light::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld)
{
    test.BeginMesh(localToWorld);

    SelectionIntersection best;

    aabb_testselect(_lightBox, test, best);

    if (best.valid())
    {
        selector.addIntersection(best);
    }
}

void Light::translate(const Vector3& translation)
{
    _originTransformed += translation;
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
        _lightStartTransformed.snap(GlobalGrid().getGridSize());
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
        _lightRightTransformed = rotationMatrix.transformPoint(_lightRight);
        _lightUpTransformed = rotationMatrix.transformPoint(_lightUp);

        if (m_useLightStart && m_useLightEnd) {
            _lightStartTransformed = rotationMatrix.transformPoint(_lightStart);
            _lightEndTransformed = rotationMatrix.transformPoint(_lightEnd);

            _lightStartTransformed.snap(GlobalGrid().getGridSize());
            _lightEndTransformed.snap(GlobalGrid().getGridSize());
        }

        // Snap the rotated vectors to the grid
        _lightRightTransformed.snap(GlobalGrid().getGridSize());
        _lightUpTransformed.snap(GlobalGrid().getGridSize());
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
        Matrix4 rotationMatrix = Matrix4::getRotation(rotation);

        // ... and apply it to all the vertices defining the projection
        _lightTargetTransformed = rotationMatrix.transformPoint(_lightTarget);
        _lightRightTransformed = rotationMatrix.transformPoint(_lightRight);
        _lightUpTransformed = rotationMatrix.transformPoint(_lightUp);
        _lightStartTransformed = rotationMatrix.transformPoint(_lightStart);
        _lightEndTransformed = rotationMatrix.transformPoint(_lightEnd);
    }
    else {
        m_rotation.rotate(rotation);
    }
}

const Matrix4& Light::getLocalPivot() const {
    m_localPivot = m_rotation.getMatrix4();
    m_localPivot.t().getVector3() = _lightBox.origin;
    return m_localPivot;
}

// greebo: This returns the AABB of the WHOLE light (this includes the volume and all its selectable vertices)
// Used to test the light for selection on mouse click.
const AABB& Light::localAABB() const
{
    if (isProjected()) {
        // start with an empty AABB and include all the projection vertices
        m_doom3AABB = AABB();
        m_doom3AABB.includePoint(_lightBox.origin);
        m_doom3AABB.includePoint(_lightBox.origin + _lightTargetTransformed);
        m_doom3AABB.includePoint(_lightBox.origin + _lightTargetTransformed + _lightRightTransformed);
        m_doom3AABB.includePoint(_lightBox.origin + _lightTargetTransformed + _lightUpTransformed);
        if (useStartEnd()) {
            m_doom3AABB.includePoint(_lightBox.origin + _lightStartTransformed);
            m_doom3AABB.includePoint(_lightBox.origin + _lightEndTransformed);
        }
    }
    else {
        m_doom3AABB = AABB(_lightBox.origin, m_doom3Radius.m_radiusTransformed);
        // greebo: Make sure the light center (that maybe outside of the light volume) is selectable
        m_doom3AABB.includePoint(_lightBox.origin + m_doom3Radius.m_centerTransformed);
    }
    return m_doom3AABB;
}

/* RendererLight implementation */
Matrix4 Light::getLightTextureTransformation() const
{
    Matrix4 world2light = Matrix4::getIdentity();

    if (isProjected())
    {
        world2light = projection();
        world2light.multiplyBy(rotation().getTransposed());
        world2light.translateBy(-getLightOrigin());
    }
    else
    {
        AABB lightBounds = lightAABB();

        world2light.translateBy(Vector3(0.5f, 0.5f, 0.5f));
        world2light.scaleBy(Vector3(0.5f, 0.5f, 0.5f));
        world2light.scaleBy(
            Vector3(1.0f / lightBounds.extents.x(),
                    1.0f / lightBounds.extents.y(),
                    1.0f / lightBounds.extents.z())
        );
        world2light.multiplyBy(rotation().getTransposed());
        world2light.translateBy(-lightBounds.origin); // world->lightBounds
    }

    return world2light;
}

/* This is needed for the drag manipulator to check the aabb of the light volume only (excl. the light center)
 */
AABB Light::lightAABB() const
{
    return AABB(_originTransformed, m_doom3Radius.m_radiusTransformed);
}

bool Light::intersectsAABB(const AABB& other) const
{
    bool returnVal;
    if (isProjected())
    {
        // Update the projection, including the Frustum (we don't care about the
        // projection matrix itself).
        projection();

        // Construct a transformation with the rotation and translation of the
        // frustum
        Matrix4 transRot = Matrix4::getIdentity();
        transRot.translateBy(worldOrigin());
        transRot.multiplyBy(rotation());

        // Transform the frustum with the rotate/translate matrix and test its
        // intersection with the AABB
        Frustum frustumTrans = _frustum.getTransformedBy(transRot);
        returnVal = frustumTrans.testIntersection(other) != VOLUME_OUTSIDE;
    }
    else
    {
        // test against an AABB which contains the rotated bounds of this light.
        AABB bounds = localAABB();
        bounds.origin += worldOrigin();

        returnVal = other.intersects(AABB(
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

    return returnVal;
}

const Matrix4& Light::rotation() const {
    m_doom3Rotation = m_rotation.getMatrix4();
    return m_doom3Rotation;
}

/* greebo: This is needed by the renderer to determine the center of the light. It returns
 * the centerTransformed variable as the lighting should be updated as soon as the light center
 * is dragged.
 */
Vector3 Light::getLightOrigin() const {
    if (isProjected())
    {
        return worldOrigin();
    }
    else
    {
        // AABB origin + light_center, i.e. center in world space
        return worldOrigin() + m_doom3Radius.m_centerTransformed;
    }
}

Vector3& Light::target()            { return _lightTarget; }
Vector3& Light::targetTransformed() { return _lightTargetTransformed; }
Vector3& Light::up()                { return _lightUp; }
Vector3& Light::upTransformed()     { return _lightUpTransformed; }
Vector3& Light::right()             { return _lightRight; }
Vector3& Light::rightTransformed()  { return _lightRightTransformed; }
Vector3& Light::start()             { return _lightStart; }
Vector3& Light::startTransformed()  { return _lightStartTransformed; }
Vector3& Light::end()               { return _lightEnd; }
Vector3& Light::endTransformed()    { return _lightEndTransformed; }

Vector3& Light::colourLightTarget() { return _colourLightTarget; }
Vector3& Light::colourLightRight()  { return _colourLightRight; }
Vector3& Light::colourLightUp()     { return _colourLightUp; }
Vector3& Light::colourLightStart()  { return _colourLightStart; }
Vector3& Light::colourLightEnd()    { return _colourLightEnd; }

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

    SceneChangeNotify();
}

// Update and return the projection matrix
const Matrix4& Light::projection() const
{
    if (!m_doom3ProjectionChanged) {
        return _projection;
    }

    m_doom3ProjectionChanged = false;

    // This transformation remaps the X,Y coordinates from [-1..1] to [0..1],
    // presumably needed because the up/right vectors extend symmetrically
    // either side of the target point.
    _projection = Matrix4::getIdentity();
    _projection.translateBy(Vector3(0.5f, 0.5f, 0));
    _projection.scaleBy(Vector3(0.5f, 0.5f, 1));

    Plane3 lightProject[4];

    float rLen = _lightRightTransformed.getLength();
    Vector3 right = _lightRightTransformed / rLen;
    float uLen = _lightUpTransformed.getLength();
    Vector3 up = _lightUpTransformed / uLen;
    Vector3 normal = up.crossProduct(right).getNormalised();

    double dist = _lightTargetTransformed.dot(normal);
    if ( dist < 0 ) {
        dist = -dist;
        normal = -normal;
    }

    right *= ( 0.5 * dist ) / rLen;
    up *= -( 0.5 * dist ) / uLen;

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

    // If there is a light_start key set, use this, otherwise use the zero
    // vector
    Vector3 start = m_useLightStart && m_useLightEnd
                    ? _lightStartTransformed
                    : Vector3(0, 0, 0);

    // If there is no light_end, but a light_start, assume light_end =
    // light_target
    Vector3 stop = m_useLightStart && m_useLightEnd
                   ? _lightEndTransformed
                   : _lightTargetTransformed;

    // Calculate the falloff vector
    Vector3 falloff = stop - start;
    float length = falloff.getLength();
    falloff /= length;
    if ( length <= 0 ) {
        length = 1;
    }
    falloff *= (1.0f / length);
    lightProject[3] = Plane3(falloff, -start.dot(falloff));

    // we want the planes of s=0, s=q, t=0, and t=q
    _frustum.left = lightProject[0];
    _frustum.bottom = lightProject[1];
    _frustum.right = Plane3(lightProject[2].normal() - lightProject[0].normal(), lightProject[2].dist() - lightProject[0].dist());
    _frustum.top = Plane3(lightProject[2].normal() - lightProject[1].normal(), lightProject[2].dist() - lightProject[1].dist());

    // we want the planes of s=0 and s=1 for front and rear clipping planes
    _frustum.front = lightProject[3];

    _frustum.back = lightProject[3];
    _frustum.back.dist() -= 1.0f;
    _frustum.back = -_frustum.back;

    // Calculate the new projection matrix from the frustum planes
    Matrix4 newProjection(_frustum.getProjectionMatrix());
    _projection.multiplyBy(newProjection);

    // Scale the falloff texture coordinate so that 0.5 is at the apex and 0.0
    // as at the base of the pyramid.
    // TODO: I don't like hacking the matrix like this, but all attempts to use
    // a transformation seemed to affect too many other things.
    _projection.zz() *= 0.5f;

    // Normalise all frustum planes
    _frustum.normalisePlanes();

    return _projection;
}

ShaderPtr Light::getShader() const {
    return m_shader.get();
}

Vector3 Light::worldOrigin() const
{
    // return the absolute world origin
    return _originTransformed;
}

} // namespace entity
