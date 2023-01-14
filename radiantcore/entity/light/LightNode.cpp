#include "LightNode.h"

#include "igrid.h"
#include "ishaders.h"
#include "icolourscheme.h"
#include "../EntitySettings.h"
#include <functional>

#include "registry/CachedKey.h"

namespace entity {

// Store the static default shader string (it's here because LightShader does not have a .cpp file)
std::string LightShader::m_defaultShader = "";

// --------- LightNode implementation ------------------------------------

LightNode::LightNode(const IEntityClassPtr& eclass)
: EntityNode(eclass), 
    m_originKey(std::bind(&LightNode::originChanged, this)),
    _originTransformed(ORIGINKEY_IDENTITY),
    m_rotationKey(std::bind(&LightNode::rotationChanged, this)),
    m_transformChanged(std::bind(&scene::Node::transformChanged, this)),
    m_boundsChanged(std::bind(&scene::Node::boundsChanged, this)),
    _instances(getDoom3Radius().m_centerTransformed, _projVectors.transformed,
               sigc::mem_fun(*this, &LightNode::selectedChangedComponent)),
    _dragPlanes(std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
    _renderableOctagon(*this, 0.5), // transparent
    _renderableOctagonOutline(*this, 1.0), // opaque lines
    _renderableLightVolume(*this),
    _renderableVertices(*this, _instances, _projUseFlags),
    _showLightVolumeWhenUnselected(EntitySettings::InstancePtr()->getShowAllLightRadii()),
    _overrideColKey(colours::RKEY_OVERRIDE_LIGHTCOL)
{
    m_doom3Radius.m_changed = std::bind(&LightNode::onLightRadiusChanged, this);
}

LightNode::LightNode(const LightNode& other)
: EntityNode(other), ILightNode(other), 
    m_originKey(std::bind(&LightNode::originChanged, this)),
    _originTransformed(ORIGINKEY_IDENTITY),
    m_rotationKey(std::bind(&LightNode::rotationChanged, this)),
    m_transformChanged(std::bind(&Node::transformChanged, this)),
    m_boundsChanged(std::bind(&Node::boundsChanged, this)),
    _instances(getDoom3Radius().m_centerTransformed, _projVectors.transformed,
        sigc::mem_fun(*this, &LightNode::selectedChangedComponent)),
    _dragPlanes(std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
    _renderableOctagon(*this, 0.5), // transparent
    _renderableOctagonOutline(*this, 1.0), // opaque lines
    _renderableLightVolume(*this),
    _renderableVertices(*this, _instances, _projUseFlags),
    _showLightVolumeWhenUnselected(other._showLightVolumeWhenUnselected),
    _overrideColKey(colours::RKEY_OVERRIDE_LIGHTCOL)
{
    m_doom3Radius.m_changed = std::bind(&LightNode::onLightRadiusChanged, this);
}

LightNodePtr LightNode::Create(const IEntityClassPtr& eclass)
{
	LightNodePtr instance(new LightNode(eclass));
	instance->construct();

	return instance;
}

void LightNode::construct()
{
	EntityNode::construct();

    m_rotation.setIdentity();
    _lightBox.origin = Vector3(0, 0, 0);
    _lightBox.extents = Vector3(8, 8, 8);
    _originTransformed = ORIGINKEY_IDENTITY;

    // Observe position and rotation spawnargs
    static_assert(std::is_base_of<sigc::trackable, OriginKey>::value);
    static_assert(std::is_base_of<sigc::trackable, RotationKey>::value);
    observeKey("origin", sigc::mem_fun(m_originKey, &OriginKey::onKeyValueChanged));
    observeKey("angle", sigc::mem_fun(m_rotationKey, &RotationKey::angleChanged));
    observeKey("rotation", sigc::mem_fun(m_rotationKey, &RotationKey::rotationChanged));

    // Observe light-specific spawnargs
    static_assert(std::is_base_of<sigc::trackable, Doom3LightRadius>::value);
    static_assert(std::is_base_of<sigc::trackable, LightNode>::value);
    static_assert(std::is_base_of<sigc::trackable, LightShader>::value);
    observeKey("light_radius",
               sigc::mem_fun(m_doom3Radius, &Doom3LightRadius::lightRadiusChanged));
    observeKey("light_center",
               sigc::mem_fun(m_doom3Radius, &Doom3LightRadius::lightCenterChanged));
    observeKey("light_rotation", sigc::mem_fun(this, &LightNode::lightRotationChanged));
    observeKey("light_target", sigc::mem_fun(this, &LightNode::lightTargetChanged));
    observeKey("light_up", sigc::mem_fun(this, &LightNode::lightUpChanged));
    observeKey("light_right", sigc::mem_fun(this, &LightNode::lightRightChanged));
    observeKey("light_start", sigc::mem_fun(this, &LightNode::lightStartChanged));
    observeKey("light_end", sigc::mem_fun(this, &LightNode::lightEndChanged));
    observeKey("texture", sigc::mem_fun(m_shader, &LightShader::valueChanged));

    _projectionChanged = true;

    _spawnArgs.setIsContainer(true);

    // Load the light colour (might be inherited)
    m_shader.valueChanged(_spawnArgs.getKeyValue("texture"));
}

const Frustum& LightNode::getLightFrustum() const
{
    if (!isProjected()) throw std::logic_error("getLightFrustum can be called on projected lights only");

    return _frustum;
}

const Vector3& LightNode::getLightStart() const
{
    if (!isProjected()) throw std::logic_error("getLightStart can be called on projected lights only");

    return _projVectors.transformed.start;
}

const Vector3& LightNode::getLightRadius() const
{
    if (isProjected()) throw std::logic_error("getLightRadius can be called on point lights only");

    return m_doom3Radius.m_radiusTransformed;
}

AABB LightNode::getSelectAABB() const
{
    // Use the light origin as select AABB centerpoint
    return AABB(getLightOrigin(), Vector3(8, 8, 8));
}

void LightNode::onLightRadiusChanged()
{
    // Light radius changed, mark bounds as dirty
    boundsChanged();
    updateRenderables();
}

void LightNode::transformChanged()
{
    EntityNode::transformChanged();

    updateRenderables();
}

float LightNode::getShaderParm(int parmNum) const
{
	return EntityNode::getShaderParm(parmNum);
}

void LightNode::onRemoveFromScene(scene::IMapRootNode& root)
{
	// Call the base class first
	EntityNode::onRemoveFromScene(root);

	// De-select all child components as well
	setSelectedComponents(false, selection::ComponentSelectionMode::Vertex);
	setSelectedComponents(false, selection::ComponentSelectionMode::Face);

    clearRenderables();
}

void LightNode::testSelect(Selector& selector, SelectionTest& test)
{
    // Generic entity selection
    EntityNode::testSelect(selector, test);

    // Light specific selection
    test.BeginMesh(localToWorld());
    SelectionIntersection best;
    aabb_testselect(_lightBox, test, best);
    if (best.isValid())
    {
        selector.addIntersection(best);
    }
}

// greebo: Returns true if drag planes or one or more light vertices are selected
bool LightNode::isSelectedComponents() const {
	return (_dragPlanes.isSelected() || _instances.center.isSelected() ||
			_instances.target.isSelected() || _instances.right.isSelected() ||
			_instances.up.isSelected() || _instances.start.isSelected() ||
			_instances.end.isSelected() );
}

// greebo: Selects/deselects all components, depending on the chosen componentmode
void LightNode::setSelectedComponents(bool select, selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Face) {
		_dragPlanes.setSelected(false);
	}

	if (mode == selection::ComponentSelectionMode::Vertex) {
		_instances.center.setSelected(false);
		_instances.target.setSelected(false);
		_instances.right.setSelected(false);
		_instances.up.setSelected(false);
		_instances.start.setSelected(false);
		_instances.end.setSelected(false);
	}
}

void LightNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
	{
		_instances.center.invertSelected();
		_instances.target.invertSelected();
		_instances.right.invertSelected();
		_instances.up.invertSelected();
		_instances.start.invertSelected();
		_instances.end.invertSelected();
	}
}

void LightNode::testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
    {
        // Use the full rotation matrix for the test
        test.BeginMesh(localToWorld());

		if (isProjected())
        {
			// Test the projection components for selection
			_instances.target.testSelect(selector, test);
			_instances.right.testSelect(selector, test);
			_instances.up.testSelect(selector, test);
			_instances.start.testSelect(selector, test);
			_instances.end.testSelect(selector, test);
		}
		else
        {
			// Test if the light center is hit by the click
			_instances.center.testSelect(selector, test);
		}
	}
}

const AABB& LightNode::getSelectedComponentsBounds() const {
	// Create a new axis aligned bounding box
	m_aabb_component = AABB();

	if (isProjected()) {
		// Include the according vertices in the AABB
		m_aabb_component.includePoint(_instances.target.getVertex());
		m_aabb_component.includePoint(_instances.right.getVertex());
		m_aabb_component.includePoint(_instances.up.getVertex());
		m_aabb_component.includePoint(_instances.start.getVertex());
		m_aabb_component.includePoint(_instances.end.getVertex());
	}
	else {
		// Just include the light center, this is the only vertex that may be out of the light volume
		m_aabb_component.includePoint(_instances.center.getVertex());
	}

	return m_aabb_component;
}

void LightNode::snapComponents(float snap) {
	if (isProjected()) {
		// Check, if any components are selected and snap the selected ones to the grid
		if (isSelectedComponents()) {
			if (_instances.target.isSelected()) {
				_projVectors.transformed.target.snap(snap);
			}
			if (_instances.right.isSelected()) {
				_projVectors.transformed.right.snap(snap);
			}
			if (_instances.up.isSelected()) {
				_projVectors.transformed.up.snap(snap);
			}

			if (useStartEnd()) {
				if (_instances.end.isSelected()) {
					_projVectors.transformed.end.snap(snap);
				}

				if (_instances.start.isSelected()) {
					_projVectors.transformed.start.snap(snap);
				}
			}
		}
		else {
			// None are selected, snap them all
			_projVectors.transformed.target.snap(snap);
			_projVectors.transformed.right.snap(snap);
			_projVectors.transformed.up.snap(snap);

			if (useStartEnd()) {
				_projVectors.transformed.end.snap(snap);
				_projVectors.transformed.start.snap(snap);
			}
		}
	}
	else {
		// There is only one vertex for point lights, namely the light_center, always snap it
		getDoom3Radius().m_centerTransformed.snap(snap);
	}

	freezeLightTransform();
}

void LightNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());
	// greebo: Make sure to use the local lightAABB() for the selection test, excluding the light center
	AABB localLightAABB(Vector3(0,0,0), getDoom3Radius().m_radiusTransformed);
	_dragPlanes.selectPlanes(localLightAABB, selector, test, selectedPlaneCallback);
}

void LightNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
{
	AABB localLightAABB(Vector3(0,0,0), getDoom3Radius().m_radiusTransformed);
	_dragPlanes.selectReversedPlanes(localLightAABB, selector, selectedPlanes);
}

scene::INodePtr LightNode::clone() const
{
	LightNodePtr node(new LightNode(*this));
	node->construct();
    node->constructClone(*this);

	return node;
}

void LightNode::selectedChangedComponent(const ISelectable& selectable)
{
	// add the selectable to the list of selected components (see RadiantSelectionSystem::onComponentSelection)
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);

    _renderableVertices.queueUpdate();
}

void LightNode::onPreRender(const VolumeTest& volume)
{
    EntityNode::onPreRender(volume);

    // Octagon faces (camera only)
    _renderableOctagon.update(_crystalFillShader);
    // Octagon outlines (for both camera and ortho)
    _renderableOctagonOutline.update(_crystalOutlineShader);

    bool lightIsSelected = isSelected();

    // Depending on the selected status or the entity settings, we need to update the wireframe volume
    if (_showLightVolumeWhenUnselected || lightIsSelected)
    {
        if (isProjected())
        {
            updateProjection();
        }

        _renderableLightVolume.update(_crystalOutlineShader);

        // Update vertices when the light is selected
        if (lightIsSelected)
        {
            _renderableVertices.setComponentMode(GlobalSelectionSystem().ComponentMode());
            _renderableVertices.update(_vertexShader);
        }
        else
        {
            _renderableVertices.clear();
        }
    }
    else
    {
        // Light volume is not visible, hide it
        _renderableLightVolume.clear();
        _renderableVertices.clear();
    }
}

void LightNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    collector.addHighlightRenderable(_renderableOctagon, Matrix4::getIdentity());
    collector.addHighlightRenderable(_renderableLightVolume, Matrix4::getIdentity());

    EntityNode::renderHighlights(collector, volume);
}

void LightNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

    // Clear the geometry from any previous shader
    clearRenderables();

    m_shader.setRenderSystem(renderSystem);

    if (renderSystem)
    {
        _vertexShader = renderSystem->capture(BuiltInShaderType::BigPoint);

        auto renderColour = getEntityColour();
        _crystalOutlineShader = renderSystem->capture(ColourShaderType::CameraAndOrthoViewOutline, renderColour);
        _crystalFillShader = renderSystem->capture(ColourShaderType::CameraTranslucent, renderColour);
        _renderableVertices.queueUpdate();
    }
    else
    {
        _crystalFillShader.reset();
        _crystalOutlineShader.reset();
        _vertexShader.reset();
    }
}

Vector4 LightNode::getEntityColour() const
{
    // Pick the colour shader according to our settings
    return _overrideColKey.get() ? EntityNode::getEntityColour() : Vector4(_colourKey.getColour(), 1.0);
}

void LightNode::evaluateTransform()
{
	if (getType() == TRANSFORM_PRIMITIVE)
    {
		translate(getTranslation());
		rotate(getRotation());
	}
	else
    {
		// Check if the light center is selected, if yes, transform it, if not, it's a drag plane operation
		if (GlobalSelectionSystem().ComponentMode() == selection::ComponentSelectionMode::Vertex)
        {
			// When the user is mouse-moving a vertex in the orthoviews he/she is operating
            // in world space. It's expected that the selected vertex follows the mouse.
            // Since the editable light vertices are measured in local coordinates
            // we have to calculate the new position in world space first and then transform
            // the point back into local space.

            if (_instances.center.isSelected())
            {
                // Retrieve the translation and apply it to the temporary light center variable
                Vector3 newWorldPos = localToWorld().transformPoint(getDoom3Radius().m_center) + getTranslation();
                getDoom3Radius().m_centerTransformed = localToWorld().getFullInverse().transformPoint(newWorldPos);
            }

			if (_instances.target.isSelected())
            {
                Vector3 newWorldPos = localToWorld().transformPoint(_projVectors.base.target) + getTranslation();
                _projVectors.transformed.target = localToWorld().getFullInverse().transformPoint(newWorldPos);
			}

            if (_instances.start.isSelected())
            {
                Vector3 newWorldPos = localToWorld().transformPoint(_projVectors.base.start) + getTranslation();
                Vector3 newLightStart = localToWorld().getFullInverse().transformPoint(newWorldPos);

                // Assign the light start, perform the boundary checks
                setLightStart(newLightStart);
            }

            if (_instances.end.isSelected())
            {
                Vector3 newWorldPos = localToWorld().transformPoint(_projVectors.base.end) + getTranslation();
                _projVectors.transformed.end = localToWorld().getFullInverse().transformPoint(newWorldPos);

                ensureLightStartConstraints();
            }

            // Even more footwork needs to be done for light_up and light_right since these
            // are measured relatively to the light_target position.

            // Extend the regular local2World by the additional light_target transform
            Matrix4 local2World = localToWorld();
            local2World.translateBy(_projVectors.base.target);
            Matrix4 world2Local = local2World.getFullInverse();

			if (_instances.right.isSelected())
            {
                Vector3 newWorldPos = local2World.transformPoint(_projVectors.base.right) + getTranslation();
                _projVectors.transformed.right = world2Local.transformPoint(newWorldPos);
			}

			if (_instances.up.isSelected())
            {
                Vector3 newWorldPos = local2World.transformPoint(_projVectors.base.up) + getTranslation();
                _projVectors.transformed.up = world2Local.transformPoint(newWorldPos);
			}

			// If this is a projected light, then it is likely for the according vertices to have changed, so update the projection
			if (isProjected())
            {
				// Call projection changed, so that the recalculation can be triggered (call for projection() would be ignored otherwise)
				projectionChanged();

				// Recalculate the frustum
                updateProjection();
			}
		}
		else
        {
			// Ordinary Drag manipulator
			// greebo: To evaluate the drag operation use a fresh AABB as starting point.
			// We don't use the aabb() or localABB() methods, those return the bounding box
            // including the light center, which may be positioned way out of the volume
            _dragPlanes.m_bounds = AABB(_originTransformed, m_doom3Radius.m_radiusTransformed);

			setLightRadius(_dragPlanes.evaluateResize(getTranslation(), rotation()));
		}
	}
}

void LightNode::_onTransformationChanged()
{
	revertLightTransform();
	evaluateTransform();
	updateOrigin();

    updateRenderables();
}

void LightNode::_applyTransformation()
{
	revertLightTransform();
	evaluateTransform();
	freezeLightTransform();
}

void LightNode::updateOrigin() {
    m_boundsChanged();

    m_doom3Radius.m_changed();

    // Update the projection as well, if necessary
    if (isProjected())
        projectionChanged();

    // Update the transformation matrix
    setLocalToParent(Matrix4::getTranslation(_originTransformed) * m_rotation.getMatrix4());

    // Notify all child nodes
    m_transformChanged();

    GlobalSelectionSystem().pivotChanged();
}

const Vector3& LightNode::getUntransformedOrigin()
{
    return m_originKey.get();
}

const Vector3& LightNode::getWorldPosition() const
{
    return _originTransformed;
}

void LightNode::onVisibilityChanged(bool isVisibleNow)
{
    EntityNode::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        updateRenderables();
    }
    else
    {
        clearRenderables();
    }
}

void LightNode::onSelectionStatusChange(bool changeGroupStatus)
{
    EntityNode::onSelectionStatusChange(changeGroupStatus);

    // Volume renderable is not always prepared for rendering, queue an update
    _renderableLightVolume.queueUpdate();
    _renderableVertices.queueUpdate();
}

void LightNode::onEntitySettingsChanged()
{
    EntityNode::onEntitySettingsChanged();

    _showLightVolumeWhenUnselected = EntitySettings::InstancePtr()->getShowAllLightRadii();
    _renderableLightVolume.queueUpdate();
}

void LightNode::originChanged()
{
    // The "origin" key has been changed, reset the current working copy to that value
    _originTransformed = m_originKey.get();
    updateOrigin();
}

void LightNode::lightTargetChanged(const std::string& value)
{
    _projUseFlags.target = (!value.empty());

    if (_projUseFlags.target)
    {
        _projVectors.base.target = string::convert<Vector3>(value);
    }

    _projVectors.transformed.target = _projVectors.base.target;
    projectionChanged();
}

void LightNode::lightUpChanged(const std::string& value)
{
    _projUseFlags.up = (!value.empty());

    if (_projUseFlags.up)
    {
        _projVectors.base.up = string::convert<Vector3>(value);
    }

    _projVectors.transformed.up = _projVectors.base.up;
    projectionChanged();
}

void LightNode::lightRightChanged(const std::string& value)
{
    _projUseFlags.right = (!value.empty());

    if (_projUseFlags.right)
    {
        _projVectors.base.right = string::convert<Vector3>(value);
    }

    _projVectors.transformed.right = _projVectors.base.right;
    projectionChanged();
}

void LightNode::lightStartChanged(const std::string& value) {
    _projUseFlags.start = (!value.empty());

    if (_projUseFlags.start)
    {
        _projVectors.base.start = string::convert<Vector3>(value);
    }

    _projVectors.transformed.start = _projVectors.base.start;

    // If the light_end key is still unused, set it to a reasonable value
    if (_projUseFlags.end) {
        checkStartEnd();
    }

    projectionChanged();
}

void LightNode::lightEndChanged(const std::string& value) {
    _projUseFlags.end = (!value.empty());

    if (_projUseFlags.end)
    {
        _projVectors.base.end = string::convert<Vector3>(value);
    }

    _projVectors.transformed.end = _projVectors.base.end;

    // If the light_start key is still unused, set it to a reasonable value
    if (_projUseFlags.start) {
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
void LightNode::checkStartEnd()
{
    if (_projUseFlags.start && _projUseFlags.end)
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

void LightNode::rotationChanged()
{
    m_rotation = m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation;

    // Update the transformation matrix
    setLocalToParent(Matrix4::getTranslation(_originTransformed) * m_rotation.getMatrix4());

    // Notify owner about this
    m_transformChanged();

    GlobalSelectionSystem().pivotChanged();
}

void LightNode::lightRotationChanged(const std::string& value) {
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
void LightNode::snapto(float snap)
{
    m_originKey.snap(snap);
    m_originKey.write(_spawnArgs);

    _originTransformed = m_originKey.get();

    updateOrigin();
}

void LightNode::setLightRadius(const AABB& aabb)
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

void LightNode::transformLightRadius(const Matrix4& transform)
{
    _originTransformed = transform.transformPoint(_originTransformed);
}

void LightNode::revertLightTransform()
{
    _originTransformed = m_originKey.get();

    m_rotation = m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation;
    m_doom3Radius.m_radiusTransformed = m_doom3Radius.m_radius;
    m_doom3Radius.m_centerTransformed = m_doom3Radius.m_center;

    // revert all the projection changes to the saved values
    _projVectors.revertTransform();
}

void LightNode::freezeLightTransform()
{
    m_originKey.set(_originTransformed);
    m_originKey.write(_spawnArgs);

    if (isProjected())
    {
        if (_projUseFlags.target)
        {
            _projVectors.base.target = _projVectors.transformed.target;
            _spawnArgs.setKeyValue("light_target",
                                string::to_string(_projVectors.base.target));
        }

        if (_projUseFlags.up)
        {
            _projVectors.base.up = _projVectors.transformed.up;
            _spawnArgs.setKeyValue("light_up",
                               string::to_string(_projVectors.base.up));
        }

        if (_projUseFlags.right)
        {
            _projVectors.base.right = _projVectors.transformed.right;
            _spawnArgs.setKeyValue("light_right",
                                string::to_string(_projVectors.base.right));
        }

        // Check the start and end (if the end is "above" the start, for example)
        checkStartEnd();

        if (_projUseFlags.start)
        {
            _projVectors.base.start = _projVectors.transformed.start;
            _spawnArgs.setKeyValue("light_start",
                                string::to_string(_projVectors.base.start));
        }

        if (_projUseFlags.end)
        {
            _projVectors.base.end = _projVectors.transformed.end;
            _spawnArgs.setKeyValue("light_end",
                                string::to_string(_projVectors.base.end));
        }
    }
    else
    {
        // Save the light center to the entity key/values
        m_doom3Radius.m_center = m_doom3Radius.m_centerTransformed;
        _spawnArgs.setKeyValue("light_center",
                            string::to_string(m_doom3Radius.m_center));
    }

    if(m_useLightRotation)
    {
        m_lightRotation = m_rotation;
        m_lightRotation.writeToEntity(&_spawnArgs, "light_rotation");
    }

    m_rotationKey.m_rotation = m_rotation;
    m_rotationKey.m_rotation.writeToEntity(&_spawnArgs);

    if (!isProjected())
    {
        m_doom3Radius.m_radius = m_doom3Radius.m_radiusTransformed;

        _spawnArgs.setKeyValue("light_radius",
                            string::to_string(m_doom3Radius.m_radius));
    }
}

Doom3LightRadius& LightNode::getDoom3Radius() {
    return m_doom3Radius;
}

void LightNode::translate(const Vector3& translation)
{
    _originTransformed += translation;
}

void LightNode::ensureLightStartConstraints()
{
    Vector3 assumedEnd = (_projUseFlags.end) ? _projVectors.transformed.end : _projVectors.transformed.target;

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

void LightNode::setLightStart(const Vector3& newLightStart)
{
    _projVectors.transformed.start = newLightStart;

    // Prevent the light_start to cause the volume form an hourglass-shaped frustum
    ensureLightStartConstraints();
}

void LightNode::rotate(const Quaternion& rotation)
{
    m_rotation.rotate(rotation);
}

// greebo: This returns the AABB of the WHOLE light (this includes the volume and all its selectable vertices)
// Used to test the light for selection on mouse click.
const AABB& LightNode::localAABB() const
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
Matrix4 LightNode::getLightTextureTransformation() const
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
AABB LightNode::lightAABB() const
{
    if (isProjected())
    {
        // Make sure our frustum is up to date
        updateProjection();

        // Return Frustum AABB in *world* space
        return _frustum.getTransformedBy(localToParent()).getAABB();
    }
    else
    {
        // AABB ignores light_center so we can't call getLightOrigin() here.
        // Just transform (0, 0, 0) by localToWorld to get the world origin for
        // the AABB.
        return AABB(localToWorld().transformPoint(Vector3(0, 0, 0)),
                    m_doom3Radius.m_radiusTransformed);
    }
}

const Matrix4& LightNode::rotation() const {
    m_doom3Rotation = m_rotation.getMatrix4();
    return m_doom3Rotation;
}

/* greebo: This is needed by the renderer to determine the center of the light. It returns
 * the centerTransformed variable as the lighting should be updated as soon as the light center
 * is dragged.
 */
Vector3 LightNode::getLightOrigin() const
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
        return localToWorld().transformPoint(
            /* (0, 0, 0) + */ m_doom3Radius.m_centerTransformed
        );
    }
}

bool LightNode::isShadowCasting() const
{
    return EntityNode::isShadowCasting();
}

bool LightNode::isBlendLight() const
{
    return m_shader.isBlendLight();
}

/* greebo: A light is projected, if the entity keys light_target/light_up/light_right are not empty.
 */
bool LightNode::isProjected() const {
    return _projUseFlags.target && _projUseFlags.up && _projUseFlags.right;
}

// greebo: Returns true if BOTH the light_start and light_end vectors are used
bool LightNode::useStartEnd() const {
    return _projUseFlags.start && _projUseFlags.end;
}

void LightNode::projectionChanged()
{
    _projectionChanged = true;
    m_doom3Radius.m_changed();

    _renderableVertices.queueUpdate();
    _renderableLightVolume.queueUpdate();

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
void LightNode::updateProjection() const
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
    Vector3 start = _projUseFlags.start && _projUseFlags.end
                    ? _projVectors.transformed.start
                    : Vector3(0, 0, 0);

    // If there is no light_end, but a light_start, assume light_end =
    // light_target
    Vector3 stop = _projUseFlags.start && _projUseFlags.end
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

const ShaderPtr& LightNode::getShader() const
{
    return m_shader.get();
}

bool LightNode::isVisible()
{
    return visible();
}

const IRenderEntity& LightNode::getLightEntity() const
{
	return *this;
}

void LightNode::onColourKeyChanged(const std::string& value)
{
    updateRenderables();
}

void LightNode::onRenderStateChanged()
{
    EntityNode::onRenderStateChanged();

    clearRenderables();
    updateRenderables();
}

void LightNode::updateRenderables()
{
    _renderableOctagon.queueUpdate();
    _renderableOctagonOutline.queueUpdate();
    _renderableLightVolume.queueUpdate();
    _renderableVertices.queueUpdate();
}

void LightNode::clearRenderables()
{
    _renderableOctagon.clear();
    _renderableOctagonOutline.clear();
    _renderableLightVolume.clear();
    _renderableVertices.clear();
}

} // namespace entity
