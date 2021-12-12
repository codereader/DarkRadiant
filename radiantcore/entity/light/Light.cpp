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
Light::Light(SpawnArgs& entity,
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
    _rCentre(m_doom3Radius.m_centerTransformed, _lightBox.origin, m_doom3Radius._centerColour),
    _rTarget(_projVectors.transformed.target, _lightBox.origin, _colourLightTarget),
    _rUp(_projVectors.transformed.up, _projVectors.transformed.target, _lightBox.origin, _colourLightUp),
    _rRight(_projVectors.transformed.right, _projVectors.transformed.target, _lightBox.origin, _colourLightRight),
    _rStart(_projVectors.transformed.start, _lightBox.origin, _colourLightStart),
    _rEnd(_projVectors.transformed.end, _lightBox.origin, _colourLightEnd),
    m_useLightRotation(false),
    m_transformChanged(transformChanged),
    m_boundsChanged(boundsChanged)
{
    m_doom3Radius.m_changed = lightRadiusChanged;
}

// Copy Constructor
Light::Light(const Light& other,
             LightNode& owner,
             SpawnArgs& entity,
             const Callback& transformChanged,
             const Callback& boundsChanged,
             const Callback& lightRadiusChanged)
: _owner(owner),
  _entity(entity),
  m_originKey(std::bind(&Light::originChanged, this)),
  _originTransformed(ORIGINKEY_IDENTITY),
  m_rotationKey(std::bind(&Light::rotationChanged, this)),
  _rCentre(m_doom3Radius.m_centerTransformed, _lightBox.origin, m_doom3Radius._centerColour),
  _rTarget(_projVectors.transformed.target, _lightBox.origin, _colourLightTarget),
  _rUp(_projVectors.transformed.up, _projVectors.transformed.target, _lightBox.origin, _colourLightUp),
  _rRight(_projVectors.transformed.right, _projVectors.transformed.target, _lightBox.origin, _colourLightRight),
  _rStart(_projVectors.transformed.start, _lightBox.origin, _colourLightStart),
  _rEnd(_projVectors.transformed.end, _lightBox.origin, _colourLightEnd),
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

    // Set the flags to their default values, before attaching the key observers,
    // which might set them to true again.
    m_useLightTarget = m_useLightUp = m_useLightRight = m_useLightStart = m_useLightEnd = false;

    // Observe position and rotation spawnargs
    static_assert(std::is_base_of<sigc::trackable, OriginKey>::value);
    static_assert(std::is_base_of<sigc::trackable, RotationKey>::value);
    _owner.observeKey("origin", sigc::mem_fun(m_originKey, &OriginKey::onKeyValueChanged));
    _owner.observeKey("angle", sigc::mem_fun(m_rotationKey, &RotationKey::angleChanged));
    _owner.observeKey("rotation", sigc::mem_fun(m_rotationKey, &RotationKey::rotationChanged));

    // Observe light-specific spawnargs
    static_assert(std::is_base_of<sigc::trackable, Doom3LightRadius>::value);
    static_assert(std::is_base_of<sigc::trackable, Light>::value);
    static_assert(std::is_base_of<sigc::trackable, LightShader>::value);
    _owner.observeKey("light_radius",
                      sigc::mem_fun(m_doom3Radius, &Doom3LightRadius::lightRadiusChanged));
    _owner.observeKey("light_center",
                      sigc::mem_fun(m_doom3Radius, &Doom3LightRadius::lightCenterChanged));
    _owner.observeKey("light_rotation", sigc::mem_fun(this, &Light::lightRotationChanged));
    _owner.observeKey("light_target", sigc::mem_fun(this, &Light::lightTargetChanged));
    _owner.observeKey("light_up", sigc::mem_fun(this, &Light::lightUpChanged));
    _owner.observeKey("light_right", sigc::mem_fun(this, &Light::lightRightChanged));
    _owner.observeKey("light_start", sigc::mem_fun(this, &Light::lightStartChanged));
    _owner.observeKey("light_end", sigc::mem_fun(this, &Light::lightEndChanged));
    _owner.observeKey("texture", sigc::mem_fun(m_shader, &LightShader::valueChanged));

    _projectionChanged = true;

    // set the colours to their default values
    m_doom3Radius.setCenterColour(_entity.getEntityClass()->getColour());

    _entity.setIsContainer(true);

    // Load the light colour (might be inherited)
    m_shader.valueChanged(_entity.getKeyValue("texture"));
}

void Light::destroy()
{
}

void Light::updateOrigin() {
    m_boundsChanged();

    m_doom3Radius.m_changed();

    // Update the projection as well, if necessary
    if (isProjected())
        projectionChanged();

    // Update the transformation matrix
    _owner.localToParent() = Matrix4::getIdentity();
    _owner.localToParent().translateBy(_originTransformed);
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
        _projVectors.base.target = string::convert<Vector3>(value);
    }

    _projVectors.transformed.target = _projVectors.base.target;
    projectionChanged();
}

void Light::lightUpChanged(const std::string& value)
{
    m_useLightUp = (!value.empty());

    if (m_useLightUp)
    {
        _projVectors.base.up = string::convert<Vector3>(value);
    }

    _projVectors.transformed.up = _projVectors.base.up;
    projectionChanged();
}

void Light::lightRightChanged(const std::string& value)
{
    m_useLightRight = (!value.empty());

    if (m_useLightRight)
    {
        _projVectors.base.right = string::convert<Vector3>(value);
    }

    _projVectors.transformed.right = _projVectors.base.right;
    projectionChanged();
}

void Light::lightStartChanged(const std::string& value) {
    m_useLightStart = (!value.empty());

    if (m_useLightStart)
    {
        _projVectors.base.start = string::convert<Vector3>(value);
    }

    _projVectors.transformed.start = _projVectors.base.start;

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
        _projVectors.base.end = string::convert<Vector3>(value);
    }

    _projVectors.transformed.end = _projVectors.base.end;

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
        if (_projVectors.base.end.getLengthSquared() < _projVectors.base.start.getLengthSquared())
        {
            // Swap the two vectors
            Vector3 temp = _projVectors.base.end;
            _projVectors.transformed.end = _projVectors.base.end = _projVectors.base.start;
            _projVectors.transformed.start = _projVectors.base.start = temp;
        }

        // The light_end on the same point as the light_start is an unlucky situation, revert it
        // otherwise the vertices won't be separable again for the user
        if (_projVectors.base.end == _projVectors.base.start)
        {
            _projVectors.transformed.end = _projVectors.base.end = _projVectors.base.target;
            _projVectors.transformed.start = _projVectors.base.start = Vector3(0,0,0);
        }
    }
}

void Light::rotationChanged()
{
    m_rotation = m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation;

    // Update the transformation matrix
    _owner.localToParent() = Matrix4::getIdentity();
    _owner.localToParent().translateBy(_originTransformed);
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
    if (EntitySettings::InstancePtr()->getDragResizeEntitiesSymmetrically())
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
    _projVectors.revertTransform();
}

void Light::freezeTransform()
{
    m_originKey.set(_originTransformed);
    m_originKey.write(_entity);

    if (isProjected())
    {
        if (m_useLightTarget)
        {
            _projVectors.base.target = _projVectors.transformed.target;
            _entity.setKeyValue("light_target",
                                string::to_string(_projVectors.base.target));
        }

        if (m_useLightUp)
        {
            _projVectors.base.up = _projVectors.transformed.up;
            _entity.setKeyValue("light_up",
                               string::to_string(_projVectors.base.up));
        }

        if (m_useLightRight)
        {
            _projVectors.base.right = _projVectors.transformed.right;
            _entity.setKeyValue("light_right",
                                string::to_string(_projVectors.base.right));
        }

        // Check the start and end (if the end is "above" the start, for example)
        checkStartEnd();

        if (m_useLightStart)
        {
            _projVectors.base.start = _projVectors.transformed.start;
            _entity.setKeyValue("light_start",
                                string::to_string(_projVectors.base.start));
        }

        if (m_useLightEnd)
        {
            _projVectors.base.end = _projVectors.transformed.end;
            _entity.setKeyValue("light_end",
                                string::to_string(_projVectors.base.end));
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

	collector.addRenderable(*_rRight.getShader(), _rRight, localToWorld);
	collector.addRenderable(*_rUp.getShader(), _rUp, localToWorld);
	collector.addRenderable(*_rTarget.getShader(), _rTarget, localToWorld);

    if (m_useLightStart)
	{
		collector.addRenderable(*_rStart.getShader(), _rStart, localToWorld);
    }

    if (m_useLightEnd)
	{
		collector.addRenderable(*_rEnd.getShader(), _rEnd, localToWorld);
    }
}

// Adds the light centre renderable to the given collector
void Light::renderLightCentre(RenderableCollector& collector,
                              const VolumeTest& volume,
                              const Matrix4& localToWorld) const
{
	collector.addRenderable(*_rCentre.getShader(), _rCentre, localToWorld);
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

void Light::translate(const Vector3& translation)
{
    _originTransformed += translation;
}

void Light::ensureLightStartConstraints()
{
    Vector3 assumedEnd = (m_useLightEnd) ? _projVectors.transformed.end : _projVectors.transformed.target;

    Vector3 normal = (_projVectors.transformed.start - assumedEnd).getNormalised();

    // Calculate the distance to the plane going through the origin, hence the minus sign
    double dist = normal.dot(_projVectors.transformed.start);

    if (dist > 0)
    {
        // Light_Start is too "high", project it back onto the origin plane
        _projVectors.transformed.start = _projVectors.transformed.start - normal*dist;
        _projVectors.transformed.start.snap(GlobalGrid().getGridSize());
    }
}

void Light::setLightStart(const Vector3& newLightStart)
{
    _projVectors.transformed.start = newLightStart;

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
        m_doom3AABB.includePoint(_lightBox.origin + _projVectors.transformed.target);
        m_doom3AABB.includePoint(_lightBox.origin + _projVectors.transformed.target + _projVectors.transformed.right);
        m_doom3AABB.includePoint(_lightBox.origin + _projVectors.transformed.target + _projVectors.transformed.up);
        if (useStartEnd()) {
            m_doom3AABB.includePoint(_lightBox.origin + _projVectors.transformed.start);
            m_doom3AABB.includePoint(_lightBox.origin + _projVectors.transformed.end);
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
        // Ensure _localToTexture matrix is up to date
        updateProjection();

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

// AABB for light volume only (excluding the light_center which might be
// outside the volume), used for drag manipulator and render culling.
AABB Light::lightAABB() const
{
    if (isProjected())
    {
        // Make sure our frustum is up to date
        updateProjection();

        // Return Frustum AABB in *world* space
        return _frustum.getTransformedBy(_owner.localToParent()).getAABB();
    }
    else
    {
        // AABB ignores light_center so we can't call getLightOrigin() here.
        // Just transform (0, 0, 0) by localToWorld to get the world origin for
        // the AABB.
        return AABB(_owner.localToWorld().transformPoint(Vector3(0, 0, 0)),
                    m_doom3Radius.m_radiusTransformed);
    }
}

const Matrix4& Light::rotation() const {
    m_doom3Rotation = m_rotation.getMatrix4();
    return m_doom3Rotation;
}

/* greebo: This is needed by the renderer to determine the center of the light. It returns
 * the centerTransformed variable as the lighting should be updated as soon as the light center
 * is dragged.
 */
Vector3 Light::getLightOrigin() const
{
    if (isProjected())
    {
        return _originTransformed;
    }
    else
    {
        // Since localToWorld() takes into account our own origin as well as the
        // transformation of any parent entity, just transform a null origin +
        // light_center by the localToWorld matrix to get the light origin in
        // world space.
        return _owner.localToWorld().transformPoint(
            /* (0, 0, 0) + */ m_doom3Radius.m_centerTransformed
        );
    }
}

Vector3& Light::target()            { return _projVectors.base.target; }
Vector3& Light::targetTransformed() { return _projVectors.transformed.target; }
Vector3& Light::up()                { return _projVectors.base.up; }
Vector3& Light::upTransformed()     { return _projVectors.transformed.up; }
Vector3& Light::right()             { return _projVectors.base.right; }
Vector3& Light::rightTransformed()  { return _projVectors.transformed.right; }
Vector3& Light::start()             { return _projVectors.base.start; }
Vector3& Light::startTransformed()  { return _projVectors.transformed.start; }
Vector3& Light::end()               { return _projVectors.base.end; }
Vector3& Light::endTransformed()    { return _projVectors.transformed.end; }

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

    auto rLen = _projVectors.transformed.right.getLength();
    Vector3 right = _projVectors.transformed.right / rLen;
    auto uLen = _projVectors.transformed.up.getLength();
    Vector3 up = _projVectors.transformed.up / uLen;
    Vector3 normal = up.cross(right).getNormalised();

    auto dist = _projVectors.transformed.target.dot(normal);
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
    Vector4 targetGlobal(_projVectors.transformed.target, 1);
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
                    ? _projVectors.transformed.start
                    : Vector3(0, 0, 0);

    // If there is no light_end, but a light_start, assume light_end =
    // light_target
    Vector3 stop = m_useLightStart && m_useLightEnd
                   ? _projVectors.transformed.end
                   : _projVectors.transformed.target;

    // Calculate the falloff vector
    Vector3 falloff = stop - start;

    auto length = falloff.getLength();
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

    const Vector3& t = _projVectors.transformed.target;
    const Vector3& u = _projVectors.transformed.up;
    const Vector3& r = _projVectors.transformed.right;

    // Scale the light volume such that it is in a [-0.5..0.5] cube, including light origin
    Vector3 boundsOrigin = (t - start) * 0.5f;
    Vector3 boundsExtents = u + r;
    boundsExtents.z() = fabs(t.z() * 0.5f);

    AABB bounds(boundsOrigin, boundsExtents);

    // Pre-calculate the local2Texture matrix which will be needed in getLightTextureTransformation()
    // The only thing missing in this matrix will be the world rotation and world translation

    // Do the mapping and mirror the z axis, we need to have q=1 at the light target plane
    auto S = Matrix4::getScale(Vector3(0.5f / bounds.extents.x(),
                                       -0.5f / bounds.extents.y(),
                                       -0.5f / bounds.extents.z()));

    // Scale the lightstart vector into the same space, we need it to calculate the projection
    double lightStart = start.getLength() * 0.5f / bounds.extents.z();
    double a = 1 / (1 - lightStart);
    double b = lightStart / (lightStart - 1);

    // This matrix projects the [-0.5..0.5] cube into the light frustum
    // It also maps the z coordinate into the [lightstart..lightend] volume
    Matrix4 projection = Matrix4::byRows(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, a, b,
        0, 0, 1, 0
    );

#if defined(DEBUG_LIGHT_MATRIX)
    using math::pp;

    std::cout << "S: " << S << "\n";
    std::cout << "projection: " << projection << "\n";
#endif

    // Now move the cube to [0..1] and we're done
    _localToTexture = Matrix4::getTranslation(Vector3(0.5f, 0.5f, 0))
                    * projection * S;

#if defined(DEBUG_LIGHT_MATRIX)

    Vector4 t4(t);
    Vector4 o(0, 0, 0, 1);
    Vector4 topRight = t + u + r;
    Vector4 bottomLeft = t - u - r;

    std::cout << "_localToTexture:" << _localToTexture
        << "\n\nTransforms:"
        << "\n\tOrigin -> " << pp(_localToTexture * o)
        << "\n\tt: " << pp(t4) << " -> " << pp(_localToTexture * t4)
        << "\n\tt + u + r: " << pp(topRight) << " -> "
                             << pp(_localToTexture * topRight)
        << "\n\tt - u - r: " << pp(bottomLeft) << " -> "
                             << pp(_localToTexture * bottomLeft)
        << "\n";
#endif
}

const ShaderPtr& Light::getShader() const
{
    return m_shader.get();
}

const IRenderEntity& Light::getLightEntity() const
{
	return _owner;
}

} // namespace entity
