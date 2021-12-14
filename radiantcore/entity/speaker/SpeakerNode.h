#pragma once

#include "../OriginKey.h"
#include "SpeakerRenderables.h"

#include "isound.h"
#include "ispeakernode.h"
#include "editable.h"
#include "inamespace.h"

#include "transformlib.h"
#include "irenderable.h"
#include "selectionlib.h"
#include "dragplanes.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"
#include "../RenderableEntityBox.h"

namespace entity
{

class SpeakerNode;
typedef std::shared_ptr<SpeakerNode> SpeakerNodePtr;

/// Entity node representing a speaker
class SpeakerNode final :
    public EntityNode,
    public Snappable,
    public PlaneSelectable,
    public ComponentSelectionTestable,
    public ISpeakerNode
{
    OriginKey m_originKey;
    Vector3 m_origin;

    // The current speaker radii (min / max)
    SoundRadii _radii;
    // The "working set" which is used during resize operations
    SoundRadii _radiiTransformed;

    // The default radii as defined on the currently active sound shader
    SoundRadii _defaultRadii;

    // The small entity box
    RenderableEntityBox _renderableBox;

    // Renderable speaker radii
    RenderableSpeakerRadiiWireframe _renderableRadiiWireframe;
    RenderableSpeakerRadiiFill _renderableRadiiFill;

    bool _showRadiiWhenUnselected;

    bool m_useSpeakerRadii;
    bool m_minIsSet;
    bool m_maxIsSet;

    AABB m_aabb_local;

    // the AABB that determines the rendering area
    AABB m_aabb_border;

    KeyObserverDelegate _radiusMinObserver;
    KeyObserverDelegate _radiusMaxObserver;
    KeyObserverDelegate _shaderObserver;

    // dragplanes for resizing using mousedrag
    selection::DragPlanes _dragPlanes;

private:
    SpeakerNode(const IEntityClassPtr& eclass);
    SpeakerNode(const SpeakerNode& other);
    void translate(const Vector3& translation);
    void revertTransform() override;
    void freezeTransform() override;
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
    void snapto(float snap) override;

    // Bounded implementation
    const AABB& localAABB() const override;

    AABB getSpeakerAABB() const override;

    // PlaneSelectable implementation
    void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) override;
    void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) override;

    // ComponentSelectionTestable implementation
    bool isSelectedComponents() const override;
    void setSelectedComponents(bool selected, selection::ComponentSelectionMode mode) override;
	void invertSelectedComponents(selection::ComponentSelectionMode mode) override;
    void testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode) override;

    // SelectionTestable implementation
    void testSelect(Selector& selector, SelectionTest& test) override;

    scene::INodePtr clone() const override;

    // Renderable implementation
    void onPreRender(const VolumeTest& volume);
    void renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const override;
    void renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume);
    void setRenderSystem(const RenderSystemPtr& renderSystem) override;

    bool isOriented() const override
    {
        return false; // speaker representation is rendered in world coordinates
    }

    void selectedChangedComponent(const ISelectable& selectable);

    void onEntitySettingsChanged() override;

    void onInsertIntoScene(scene::IMapRootNode& root) override;
    void onRemoveFromScene(scene::IMapRootNode& root) override;

protected:
    // Gets called by the Transformable implementation whenever
    // scale, rotation or translation is changed.
    void _onTransformationChanged() override;

    // Called by the Transformable implementation before freezing
    // or when reverting transformations.
    void _applyTransformation() override;

    // Called after the constructor is done, overrides EntityNode
    void construct() override;

    void onSelectionStatusChange(bool changeGroupStatus) override;

private:
    void evaluateTransform();
};

} // namespace
