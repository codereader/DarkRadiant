#include "Light.h"

#include "iradiant.h"
#include "itextstream.h"
#include "igrid.h"
#include "Doom3LightRadius.h"
#include "LightShader.h"
#include <functional>
#include "../EntitySettings.h"

#include "LightNode.h"

namespace entity
{

// Initialise the static default shader string
std::string LightShader::m_defaultShader = "";

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
    m_originKey(std::bind(&Light::originChanged, this)),
    _originTransformed(ORIGINKEY_IDENTITY),
    m_rotationKey(std::bind(&Light::rotationChanged, this)),
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
  m_originKey(std::bind(&Light::originChanged, this)),
  _originTransformed(ORIGINKEY_IDENTITY),
  m_rotationKey(std::bind(&Light::rotationChanged, this)),
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

    _angleObserver.setCallback(std::bind(&RotationKey::angleChanged, &m_rotationKey, std::placeholders::_1));
    _rotationObserver.setCallback(std::bind(&RotationKey::rotationChanged, &m_rotationKey, std::placeholders::_1));

    _lightRadiusObserver.setCallback(std::bind(&Doom3LightRadius::lightRadiusChanged, &m_doom3Radius, std::placeholders::_1));
    _lightCenterObserver.setCallback(std::bind(&Doom3LightRadius::lightCenterChanged, &m_doom3Radius, std::placeholders::_1));
    _lightRotationObserver.setCallback(std::bind(&Light::lightRotationChanged, this, std::placeholders::_1));
    _lightTargetObserver.setCallback(std::bind(&Light::lightTargetChanged, this, std::placeholders::_1));
    _lightUpObserver.setCallback(std::bind(&Light::lightUpChanged, this, std::placeholders::_1));
    _lightRightObserver.setCallback(std::bind(&Light::lightRightChanged, this, std::placeholders::_1));
    _lightStartObserver.setCallback(std::bind(&Light::lightStartChanged, this, std::placeholders::_1));
    _lightEndObserver.setCallback(std::bind(&Light::lightEndChanged, this, std::placeholders::_1));
    _lightTextureObserver.setCallback(std::bind(&LightShader::valueChanged, &m_shader, std::placeholders::_1));

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

    _projectionChanged = true;

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

const Vector3& Light::getUntransformedOrigin() const
{
    return m_originKey.get();
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
void Light::checkStartEnd()
{
    if (m_useLightStart && m_useLightEnd)
    {
        if (_lightEnd.getLengthSquared() < _lightStart.getLengthSquared())
        {
            // Swap the two vectors
            Vector3 temp = _lightEnd;
            _lightEndTransformed = _lightEnd = _lightStart;
            _lightStartTransformed = _lightStart = temp;
        }

        // The light_end on the same point as the light_start is an unlucky situation, revert it
        // otherwise the vertices won't be separable again for the user
        if (_lightEnd == _lightStart)
        {
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
void Light::render(const RenderInfo& info) const
{
    // Revert the light "diamond" to default extents for drawing
    AABB tempAABB(_lightBox.origin, Vector3(8, 8, 8));

    // Calculate the light vertices of this bounding box and store them into <points>
    Vector3 max(tempAABB.origin + tempAABB.extents);
    Vector3 min(tempAABB.origin - tempAABB.extents);
    Vector3 mid(tempAABB.origin);

    // top, bottom, tleft, tright, bright, bleft
    Vector3 points[6] =
    {
        Vector3(mid[0], mid[1], max[2]),
        Vector3(mid[0], mid[1], min[2]),
        Vector3(min[0], max[1], mid[2]),
        Vector3(max[0], max[1], mid[2]),
        Vector3(max[0], min[1], mid[2]),
        Vector3(min[0], min[1], mid[2])
    };

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
    glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(index_t), RenderIndexTypeID, indices);
}

Doom3LightRadius& Light::getDoom3Radius() {
    return m_doom3Radius;
}

void Light::renderProjectionPoints(RenderableCollector& collector,
                                   const VolumeTest& volume,
                                   const Matrix4& localToWorld) const 
{
    // Add the renderable light target
    collector.setHighlightFlag(RenderableCollector::Highlight::Primitives, false);
    collector.setHighlightFlag(RenderableCollector::Highlight::Faces, false);

	collector.addRenderable(_rRight.getShader(), _rRight, localToWorld);
	collector.addRenderable(_rUp.getShader(), _rUp, localToWorld);
	collector.addRenderable(_rTarget.getShader(), _rTarget, localToWorld);

    if (m_useLightStart)
	{
		collector.addRenderable(_rStart.getShader(), _rStart, localToWorld);
    }

    if (m_useLightEnd)
	{
		collector.addRenderable(_rEnd.getShader(), _rEnd, localToWorld);
    }
}

// Adds the light centre renderable to the given collector
void Light::renderLightCentre(RenderableCollector& collector,
                              const VolumeTest& volume,
                              const Matrix4& localToWorld) const 
{
	collector.addRenderable(_rCentre.getShader(), _rCentre, localToWorld);
}

void Light::renderWireframe(RenderableCollector& collector,
                            const VolumeTest& volume,
                            const Matrix4& localToWorld,
                            bool selected) const
{
    // Main render, submit the diamond that represents the light entity
    collector.addRenderable(_owner.getColourShader(), *this, localToWorld);

    // Render bounding box if selected or the showAllLighRadii flag is set
    if (selected || EntitySettings::InstancePtr()->showAllLightRadii())
    {
        if (isProjected())
        {
            // greebo: This is not much of an performance impact as the
            // projection gets only recalculated when it has actually changed.
            updateProjection();
            collector.addRenderable(_owner.getColourShader(), _renderableFrustum, localToWorld);
        }
        else
        {
            updateRenderableRadius();
            collector.addRenderable(_owner.getColourShader(), _renderableRadius, localToWorld);
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

    if (best.isValid())
    {
        selector.addIntersection(best);
    }
}

void Light::translate(const Vector3& translation)
{
    _originTransformed += translation;
}

void Light::ensureLightStartConstraints()
{
    Vector3 assumedEnd = (m_useLightEnd) ? _lightEndTransformed : _lightTargetTransformed;

    Vector3 normal = (_lightStartTransformed - assumedEnd).getNormalised();

    // Calculate the distance to the plane going through the origin, hence the minus sign
    double dist = normal.dot(_lightStartTransformed);

    if (dist > 0)
    {
        // Light_Start is too "high", project it back onto the origin plane
        _lightStartTransformed = _lightStartTransformed - normal*dist;
        _lightStartTransformed.snap(GlobalGrid().getGridSize());
    }
}

void Light::setLightStart(const Vector3& newLightStart)
{
    _lightStartTransformed = newLightStart;

    // Prevent the light_start to cause the volume form an hourglass-shaped frustum
    ensureLightStartConstraints();
}

void Light::rotate(const Quaternion& rotation)
{
    m_rotation.rotate(rotation);
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
    // greebo: Some notes on the world2Light matrix
    // This matrix transforms a world point (i.e. relative to the 0,0,0 world origin)
    // into texture coordinates that span the range [0..1] within the light volume.

    // Example:
    // For non-rotated point lights the world point [origin - light_radius] will be 
    // transformed to [0,0,0], whereas [origin + light_radius] will be [1,1,1]

    if (isProjected())
    {
        // First step: subtract the light origin from the world point
        Matrix4 worldTolight = Matrix4::getTranslation(-getLightOrigin());

        // "Undo" the light rotation, we're now in local space
        worldTolight.premultiplyBy(rotation().getTransposed());

        // Transform the local coordinates into texture space and we're done
        worldTolight.premultiplyBy(_localToTexture);

        return worldTolight;
    }
    else // point light
    {
        AABB lightBounds = lightAABB();

        // First step: subtract the light origin from the world point
        Matrix4 worldTolight = Matrix4::getTranslation(-lightBounds.origin);

        // "Undo" the light rotation
        worldTolight.premultiplyBy(rotation().getTransposed());

        // Map the point to a small [-1..1] cube around the origin
        worldTolight.premultiplyBy(Matrix4::getScale(
            Vector3(1.0f / lightBounds.extents.x(),
                    1.0f / lightBounds.extents.y(),
                    1.0f / lightBounds.extents.z())
        ));
        // To get texture coordinates in the range of [0..1], we need to scale down 
        // one more time. [-1..1] is 2 units wide, so scale down by factor 2.
        // By this time, points within the light volume have been mapped 
        // into a [-0.5..0.5] cube around the origin.
        worldTolight.premultiplyBy(Matrix4::getScale(Vector3(0.5f, 0.5f, 0.5f)));

        // Now move the [-0.5..0.5] cube to [0..1] and we're done
        worldTolight.premultiplyBy(Matrix4::getTranslation(Vector3(0.5f, 0.5f, 0.5f)));

        return worldTolight;
    }
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
        updateProjection();

        // Construct a transformation with the rotation and translation of the
        // frustum
        Matrix4 transRot = Matrix4::getIdentity();
        transRot.translateBy(worldOrigin());
        transRot.multiplyBy(rotation());

        // Transform the frustum with the rotate/translate matrix and test its
        // intersection with the AABB
		Frustum frustumTrans = _frustum.getTransformedBy(transRot);

		VolumeIntersectionValue intersects = frustumTrans.testIntersection(other);

        returnVal = intersects != VOLUME_OUTSIDE;
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
    _projectionChanged = true;
    m_doom3Radius.m_changed();

    SceneChangeNotify();
}

/**
* greebo: In TDM / Doom3, the idPlane object stores the plane's a,b,c,d
* coefficients, in DarkRadiant, the fourth number in Plane3 is dist, which is -d
* Previously, this routine just hard-cast the Plane3 object to a Vector4
* which is wrong due to the fourth number being negated.
*/
inline BasicVector4<double> plane3_to_vector4(const Plane3& self)
{
	return BasicVector4<double>(self.normal(), -self.dist());
}

// Update and return the projection matrix
void Light::updateProjection() const
{
    if (!_projectionChanged)
    {
        return;
    }

    _projectionChanged = false;

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

		lightProject[0].normal() += lightProject[2].normal() * ofs;
		lightProject[0].dist() -= lightProject[2].dist() * ofs;
        //plane3_to_vector4(lightProject[0]) += plane3_to_vector4(lightProject[2]) * ofs;
    }
    {
        double a = targetGlobal.dot(plane3_to_vector4(lightProject[1]));
        double b = targetGlobal.dot(plane3_to_vector4(lightProject[2]));
        double ofs = 0.5 - a / b;

		lightProject[1].normal() += lightProject[2].normal() * ofs;
		lightProject[1].dist() -= lightProject[2].dist() * ofs;
        //plane3_to_vector4(lightProject[1]) += plane3_to_vector4(lightProject[2]) * ofs;
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
    lightProject[3] = Plane3(falloff, start.dot(falloff));

	//rMessage() << "Light at " << m_originKey.get() << std::endl;
	//
	//for (int i = 0; i < 4; ++i)
	//{
	//	rMessage() << "  Plane " << i << ": " << lightProject[i].normal() << ", dist: " << lightProject[i].dist() << std::endl;
	//}
	
	// greebo: Comparing this to the engine sources, all frustum planes in TDM 
	// appear to be negated, their normals are pointing outwards.

    // we want the planes of s=0, s=q, t=0, and t=q
    _frustum.left = -lightProject[0];
    _frustum.top = -lightProject[1];
	_frustum.right = -(lightProject[2] - lightProject[0]);
	_frustum.bottom = -(lightProject[2] - lightProject[1]);

    // we want the planes of s=0 and s=1 for front and rear clipping planes
    _frustum.front = -lightProject[3];

	_frustum.back = lightProject[3];
	_frustum.back.dist() += 1.0f;

    // For intersection tests, we want a frustum with all plane normals pointing inwards
    _frustum.left.reverse();
    _frustum.right.reverse();
    _frustum.top.reverse();
    _frustum.bottom.reverse();
    _frustum.back.reverse();
    _frustum.front.reverse();

    // Normalise all frustum planes
    _frustum.normalisePlanes();

	// TDM uses an array of 6 idPlanes, these relate to DarkRadiant like this: 
	// 0 = left, 1 = top, 2 = right, 3 = bottom, 4 = front, 5 = back
	//rMessage() << "  Frustum Plane " << 0 << ": " << _frustum.left.normal() << ", dist: " << _frustum.left.dist() << std::endl;
	//rMessage() << "  Frustum Plane " << 1 << ": " << _frustum.top.normal() << ", dist: " << _frustum.top.dist() << std::endl;
	//rMessage() << "  Frustum Plane " << 2 << ": " << _frustum.right.normal() << ", dist: " << _frustum.right.dist() << std::endl;
	//rMessage() << "  Frustum Plane " << 3 << ": " << _frustum.bottom.normal() << ", dist: " << _frustum.bottom.dist() << std::endl;
	//rMessage() << "  Frustum Plane " << 4 << ": " << _frustum.front.normal() << ", dist: " << _frustum.front.dist() << std::endl;
	//rMessage() << "  Frustum Plane " << 5 << ": " << _frustum.back.normal() << ", dist: " << _frustum.back.dist() << std::endl;

    // Pre-calculate the local2Texture matrix which will be needed in getLightTextureTransformation()
    // The only thing missing in this matrix will be the world rotation and world translation
    _localToTexture = Matrix4::getIdentity();

    // Scale the light volume such that it is in a [-0.5..0.5] cube, including light origin
    Vector3 boundsOrigin = (_lightTargetTransformed - _lightStartTransformed) * 0.5f;
    Vector3 boundsExtents = _lightUpTransformed + _lightRightTransformed;
    boundsExtents.z() = fabs(_lightTargetTransformed.z() * 0.5f);

    AABB bounds(boundsOrigin, boundsExtents);

    // Do the mapping and mirror the z axis, we need to have q=1 at the light target plane
    _localToTexture.premultiplyBy(Matrix4::getScale(
        Vector3(0.5f / bounds.extents.x(),
        -0.5f / bounds.extents.y(),
        -0.5f / bounds.extents.z())
        ));

    // Scale the lightstart vector into the same space, we need it to calculate the projection
    double lightStart = _lightStartTransformed.getLength() * 0.5f / bounds.extents.z();
    double a = 1 / (1 - lightStart);
    double b = lightStart / (lightStart - 1);

    // This matrix projects the [-0.5..0.5] cube into the light frustum
    // It also maps the z coordinate into the [lightstart..lightend] volume
    Matrix4 projection = Matrix4::byColumns(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, a, 1,
        0, 0, b, 0
    );

    _localToTexture.premultiplyBy(projection);

    // Now move the cube to [0..1] and we're done
    _localToTexture.premultiplyBy(Matrix4::getTranslation(Vector3(0.5f, 0.5f, 0)));
}

const ShaderPtr& Light::getShader() const
{
    return m_shader.get();
}

const Vector3& Light::worldOrigin() const
{
    // return the absolute world origin
    return _originTransformed;
}

} // namespace entity
