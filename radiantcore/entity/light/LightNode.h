#pragma once

#include "Doom3LightRadius.h"
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
#include "Renderables.h"
#include "LightVertexInstanceSet.h"

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
    public RendererLight
{
	OriginKey m_originKey;
	// The "working" version of the origin
	Vector3 _originTransformed;

    RotationKey m_rotationKey;
    RotationMatrix m_rotation;

	Doom3LightRadius m_doom3Radius;

    RotationMatrix m_lightRotation;
    bool m_useLightRotation = false;

    // Projected light vectors, both base and transformed
    scene::TransformedCopy<Projected<Vector3>> _projVectors;

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
    ShaderPtr _vertexShader;
    ShaderPtr _crystalFillShader;
    ShaderPtr _crystalOutlineShader;

    // The 8x8 box representing the light object itself
    AABB _lightBox;

    Callback m_transformChanged;
    Callback m_boundsChanged;
    Callback m_evaluateTransform;

    LightVertexInstanceSet _instances;

	// dragplanes for lightresizing using mousedrag
    selection::DragPlanes _dragPlanes;

	// Renderable components of this light
    RenderableLightOctagon _renderableOctagon;
    RenderableLightOctagon _renderableOctagonOutline;
    RenderableLightVolume _renderableLightVolume;
    RenderableLightVertices _renderableVertices;

    bool _showLightVolumeWhenUnselected;

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

    void transformChanged() override;

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
    void onPreRender(const VolumeTest& volume) override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	const Matrix4& rotation() const;

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;

    const Vector3& getWorldPosition() const override;

    void onEntitySettingsChanged() override;

    // Is this light projected or omni?
    bool isProjected() const;

    // Returns the frustum structure (calling this on point lights will throw)
    const Frustum& getLightFrustum() const;

    // Returns the relative start point used by projected lights to cut off
    // the upper part of the projection cone to form the frustum
    // Calling this on point lights will throw.
    const Vector3& getLightStart() const;

    // Returns the light radius for point lights
    // Calling this on projected lights will throw
    const Vector3& getLightRadius() const;

    virtual Vector4 getEntityColour() const override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
    void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

	// Override EntityNode::construct()
	void construct() override;

    void onVisibilityChanged(bool isVisibleNow) override;
    void onSelectionStatusChange(bool changeGroupStatus) override;

    void onColourKeyChanged(const std::string& value) override;

private:
    void evaluateTransform();

    // Ensure the start and end points are set to sensible values
	void checkStartEnd();

	void updateOrigin();
	void originChanged();
	void lightTargetChanged(const std::string& value);
	void lightUpChanged(const std::string& value);
	void lightRightChanged(const std::string& value);
	void lightStartChanged(const std::string& value);
	void lightEndChanged(const std::string& value);
	void rotationChanged();
	void lightRotationChanged(const std::string& value);
    void onLightRadiusChanged();

	// Returns a reference to the member class Doom3LightRadius (used to set colours)
	Doom3LightRadius& getDoom3Radius();

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

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	void setLightRadius(const AABB& aabb);
	void transformLightRadius(const Matrix4& transform);
	void revertLightTransform();
	void freezeLightTransform();

    // Set the projection-changed flag
	void projectionChanged();

    // Update the projected light frustum
    void updateProjection() const;
	bool useStartEnd() const;

    void updateRenderables();
    void clearRenderables();

public:
    // RendererLight implementation
    const IRenderEntity& getLightEntity() const override;
    Matrix4 getLightTextureTransformation() const override;
    Vector3 getLightOrigin() const override;
    bool isShadowCasting() const override;
    const ShaderPtr& getShader() const override;
	AABB lightAABB() const override;
};

} // namespace entity
