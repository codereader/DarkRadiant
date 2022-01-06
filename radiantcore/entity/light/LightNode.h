#pragma once

#include "Doom3LightRadius.h"
#include "RenderableVertices.h"
#include "Renderables.h"
#include "LightShader.h"

#include "ilightnode.h"
#include "registry/CachedKey.h"
#include "scene/TransformedCopy.h"

#include "dragplanes.h"
#include "../VertexInstance.h"
#include "../EntityNode.h"
#include "../OriginKey.h"
#include "../RotationKey.h"

namespace entity
{

class LightNode;
typedef std::shared_ptr<LightNode> LightNodePtr;

/// Scenegraph node representing a light
class LightNode :
    public EntityNode,
    public ILightNode,
    public Snappable,
    public ComponentSelectionTestable,
    public ComponentEditable,
    public ComponentSnappable,
    public PlaneSelectable,
    public OpenGLRenderable,
    public RendererLight
{
	OriginKey m_originKey;
	// The "working" version of the origin
	Vector3 _originTransformed;

    RotationKey m_rotationKey;
    RotationMatrix m_rotation;

	Doom3LightRadius m_doom3Radius;

	// Renderable components of this light
	RenderableLightTarget _rCentre;
	RenderableLightTarget _rTarget;

	RenderableLightRelative _rUp;
	RenderableLightRelative _rRight;

	RenderableLightTarget _rStart;
	RenderableLightTarget _rEnd;

    RotationMatrix m_lightRotation;
    bool m_useLightRotation = false;

    // Set of values defining a projected light
    template<typename T> struct Projected
    {
        T target;
        T up;
        T right;
        T start;
        T end;
    };

    // Projected light vectors, both base and transformed
    scene::TransformedCopy<Projected<Vector3>> _projVectors;

    // Projected light vector colours
    Projected<Vector3> _projColours;

    // Projected light use flags
    Projected<bool> _projUseFlags;

    mutable AABB m_doom3AABB;
    mutable Matrix4 m_doom3Rotation;

    // Frustum for projected light (used for rendering the light volume)
    mutable Frustum _frustum;

    // Transforms local space coordinates into texture coordinates
    // To get the complete texture transform this one needs to be
    // post-multiplied by the world rotation and translation.
    mutable Matrix4 _localToTexture;

    mutable bool _projectionChanged;

	LightShader m_shader;

    // The 8x8 box representing the light object itself
    AABB _lightBox;

    Callback m_transformChanged;
    Callback m_boundsChanged;
    Callback m_evaluateTransform;

	// The (draggable) light center instance
	VertexInstance _lightCenterInstance;

	VertexInstance _lightTargetInstance;
	VertexInstanceRelative _lightRightInstance;
	VertexInstanceRelative _lightUpInstance;
	VertexInstance _lightStartInstance;
	VertexInstance _lightEndInstance;

	// dragplanes for lightresizing using mousedrag
    selection::DragPlanes _dragPlanes;

	// Renderable components of this light
	RenderLightRadiiBox _renderableRadius;
    RenderLightProjection _renderableFrustum;

	// a temporary variable for calculating the AABB of all (selected) components
	mutable AABB m_aabb_component;

    // Cached rkey to override light volume colour
    registry::CachedKey<bool> _overrideColKey;

	mutable Matrix4 m_projectionOrientation;

public:
	LightNode(const IEntityClassPtr& eclass);

private:
	LightNode(const LightNode& other);

public:
	static LightNodePtr Create(const IEntityClassPtr& eclass);

    // ILightNode implementation
    const RendererLight& getRendererLight() const override { return *this; }

	// RenderEntity implementation
	virtual float getShaderParm(int parmNum) const override;

	// Bounded implementation
	const AABB& localAABB() const override;

	// override scene::Node methods to deselect the child components
	virtual void onRemoveFromScene(scene::IMapRootNode& root) override;

	// Snappable implementation
	void snapto(float snap) override;

	/** greebo: Returns the AABB of the small diamond representation.
	 *	(use this to select the light against an AABB selectiontest like CompleteTall or similar).
	 */
	AABB getSelectAABB() const override;

	/* greebo: This snaps the components to the grid.
	 *
	 * Note: if none are selected, ALL the components are snapped to the grid (I hope this is intentional)
	 * This function can only be called in Selection::eVertex mode, so I assume that the user wants all components
	 * to be snapped.
	 *
	 * If one or more components is/are selected, ONLY those are snapped to the grid.
	 */
	void snapComponents(float snap) override;

	// PlaneSelectable implementation
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) override;
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) override;

	// Test the light volume for selection, this just passes the call on to the contained Light class
	void testSelect(Selector& selector, SelectionTest& test) override;

	// greebo: Returns true if drag planes or the light center is selected (both are components)
	bool isSelectedComponents() const override;
	// greebo: Selects/deselects all components, depending on the chosen componentmode
	void setSelectedComponents(bool select, selection::ComponentSelectionMode mode) override;
	void invertSelectedComponents(selection::ComponentSelectionMode mode) override;
	void testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode) override;

	/**
	 * greebo: This returns the AABB of all the selectable vertices. This method
	 * distinguishes between projected and point lights and stretches the AABB accordingly.
	 */
	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const override;

	scene::INodePtr clone() const override;
	void selectedChangedComponent(const ISelectable& selectable);

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;
	void renderComponents(RenderableCollector& collector, const VolumeTest& volume) const override;

    // OpenGLRenderable implementation
    void render(const RenderInfo& info) const override;

	const Matrix4& rotation() const;

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
    void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

	// Override EntityNode::construct()
	void construct() override;

private:
    void renderInactiveComponents(RenderableCollector& collector, const VolumeTest& volume, const bool selected) const;
    void evaluateTransform();

    // Render the light volume including bounds and origin
    void renderLightVolume(RenderableCollector& collector,
                           const Matrix4& localToWorld, bool selected) const;

    // Update the bounds of the renderable radius box
    void updateRenderableRadius() const;

    void onLightRadiusChanged();

private: // Light methods
	void destroy();

    // Ensure the start and end points are set to sensible values
	void checkStartEnd();

	void updateOrigin();
	void originChanged();
	void lightTargetChanged(const std::string& value);
	void lightUpChanged(const std::string& value);
	void lightRightChanged(const std::string& value);
	void lightStartChanged(const std::string& value);
	void lightEndChanged(const std::string& value);
	void writeLightOrigin();
	void rotationChanged();
	void lightRotationChanged(const std::string& value);

	// Renderable submission functions
	void renderWireframe(RenderableCollector& collector,
						 const VolumeTest& volume,
						 const Matrix4& localToWorld,
						 bool selected) const;

	// Adds the light centre renderable to the given collector
	void renderLightCentre(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderProjectionPoints(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;

	// Returns a reference to the member class Doom3LightRadius (used to set colours)
	Doom3LightRadius& getDoom3Radius();

	void translate(const Vector3& translation);

    /**
     * greebo: This sets the light start to the given value, including bounds checks.
     */
	void setLightStart(const Vector3& newLightStart);

    /**
     * greebo: Checks if the light_start is positioned "above" the light origin and constrains
     * the movement accordingly to prevent the light volume to become an "hourglass".
     * Only affects the _lightStartTransformed member.
     */
    void ensureLightStartConstraints();

	void rotate(const Quaternion& rotation);

	// This snaps the light as a whole to the grid (basically the light origin)
	void setLightRadius(const AABB& aabb);
	void transformLightRadius(const Matrix4& transform);
	void revertLightTransform();
	void freezeLightTransform();

    // Is this light projected or omni?
    bool isProjected() const;

    // Set the projection-changed flag
	void projectionChanged();

    // Update the projected light frustum
    void updateProjection() const;

    // RendererLight implementation
    const IRenderEntity& getLightEntity() const override;
    Matrix4 getLightTextureTransformation() const override;
    Vector3 getLightOrigin() const override;
    const ShaderPtr& getShader() const override;
	AABB lightAABB() const override;

	Vector3& colourLightTarget();
	Vector3& colourLightRight();
	Vector3& colourLightUp();
	Vector3& colourLightStart();
	Vector3& colourLightEnd();

	bool useStartEnd() const;

}; // class LightNode

} // namespace entity
