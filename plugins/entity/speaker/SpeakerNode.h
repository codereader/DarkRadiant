#pragma once

#include "../OriginKey.h"
#include "SpeakerRenderables.h"

#include "isound.h"
#include "editable.h"
#include "inamespace.h"

#include "transformlib.h"
#include "irenderable.h"
#include "selectionlib.h"
#include "dragplanes.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"

namespace entity
{

class SpeakerNode;
typedef boost::shared_ptr<SpeakerNode> SpeakerNodePtr;

/// Entity node representing a speaker
class SpeakerNode :
    public EntityNode,
    public Snappable,
    public PlaneSelectable,
    public ComponentSelectionTestable
{
    OriginKey m_originKey;
    Vector3 m_origin;

    // The current speaker radii (min / max)
    SoundRadii _radii;
    // The "working set" which is used during resize operations
    SoundRadii _radiiTransformed;

    // The default radii as defined on the currently active sound shader
    SoundRadii _defaultRadii;

    // Renderable speaker radii
    RenderableSpeakerRadii _renderableRadii;

    bool m_useSpeakerRadii;
    bool m_minIsSet;
    bool m_maxIsSet;

    AABB m_aabb_local;

    // the AABB that determines the rendering area
    AABB m_aabb_border;

    RenderableSolidAABB m_aabb_solid;
    RenderableWireframeAABB m_aabb_wire;

    KeyObserverDelegate _radiusMinObserver;
    KeyObserverDelegate _radiusMaxObserver;
    KeyObserverDelegate _shaderObserver;

    // dragplanes for resizing using mousedrag
    DragPlanes _dragPlanes;

private:
    SpeakerNode(const IEntityClassPtr& eclass);
    SpeakerNode(const SpeakerNode& other);
    void translate(const Vector3& translation);
    void rotate(const Quaternion& rotation);
    void revertTransform();
    void freezeTransform();
    void updateTransform();
    void updateAABB();
    void originChanged();
    void sShaderChanged(const std::string& value);
    void sMinChanged(const std::string& value);
    void sMaxChanged(const std::string& value);

    // greebo: Modifies the speaker radii according to the passed bounding box
    // this is called during drag-resize operations
    void setRadiusFromAABB(const AABB& aabb);

public:

    /// Public construction function
    static SpeakerNodePtr create(const IEntityClassPtr& eclass);
    
    ~SpeakerNode();

    // Snappable implementation
    void snapto(float snap);

    // Bounded implementation
    const AABB& localAABB() const;

    // PlaneSelectable implementation
    void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
    void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);

    // ComponentSelectionTestable implementation
    bool isSelectedComponents() const;
    void setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode);
    void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

    // SelectionTestable implementation
    void testSelect(Selector& selector, SelectionTest& test);

    scene::INodePtr clone() const;

    // Renderable implementation
    void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
    void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

    void selectedChangedComponent(const Selectable& selectable);

protected:
    // Gets called by the Transformable implementation whenever
    // scale, rotation or translation is changed.
    void _onTransformationChanged();

    // Called by the Transformable implementation before freezing
    // or when reverting transformations.
    void _applyTransformation();

    // Called after the constructor is done, overrides EntityNode
    void construct();

private:
    void evaluateTransform();
};

} // namespace entity
