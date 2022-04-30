#pragma once

#include "ibrush.h"
#include "itraceable.h"
#include "iscenegraph.h"
#include "icomparablenode.h"

#include "Brush.h"
#include "scene/SelectableNode.h"
#include "FaceInstance.h"
#include "EdgeInstance.h"
#include "VertexInstance.h"
#include "BrushClipPlane.h"
#include "transformlib.h"
#include "scene/Node.h"
#include "RenderableBrushVertices.h"

class BrushNode :
	public scene::SelectableNode,
	public scene::Cloneable,
	public Snappable,
	public IdentityTransform,
	public Translatable,
	public IBrushNode,
	public BrushObserver,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public PlaneSelectable,
	public Transformable,
	public ITraceable,
    public scene::IComparableNode
{
	// The actual contained brush (NO reference)
	Brush m_brush;

	FaceInstances m_faceInstances;

	typedef std::vector<EdgeInstance> EdgeInstances;
	EdgeInstances m_edgeInstances;
	typedef std::vector<brush::VertexInstance> VertexInstances;
	VertexInstances m_vertexInstances;

    // All selectable points (corner vertices / edge or face centroids)
	std::vector<Vector3> _selectedPoints;

	mutable AABB m_aabb_component;
	BrushClipPlane m_clipPlane;

	ShaderPtr _pointShader;

	// TRUE if any of the FaceInstance's component selection got changed or transformed
	bool _renderableComponentsNeedUpdate;
    std::size_t _numSelectedComponents;

    // For pivoted rotations, we need a copy of this lying around
    Vector3 _untransformedOrigin;
    // If true, the _untransformedOrigin member needs an update
    bool _untransformedOriginChanged;

    brush::RenderableBrushVertices _renderableVertices;

    bool _facesNeedRenderableUpdate;

public:
	// Constructor
	BrushNode();

	// Copy Constructor
	BrushNode(const BrushNode& other);

	virtual ~BrushNode();

	// IBrushNode implementation
	virtual Brush& getBrush() override;
	virtual IBrush& getIBrush() override;

	std::string name() const  override
    {
		return "Brush";
	}

	Type getNodeType() const override;

    // IComparable implementation
    std::string getFingerprint() override;

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	// SelectionTestable implementation
	virtual void testSelect(Selector& selector, SelectionTest& test) override;

	// ComponentSelectionTestable
	bool isSelectedComponents() const override;
	void setSelectedComponents(bool select, selection::ComponentSelectionMode mode) override;
	void invertSelectedComponents(selection::ComponentSelectionMode mode) override;
	void testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode) override;

	// override scene::Inode::onRemoveFromScene to deselect the child components
    virtual void onInsertIntoScene(scene::IMapRootNode& root) override;
    virtual void onRemoveFromScene(scene::IMapRootNode& root) override;

	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const override;

	void selectedChangedComponent(const ISelectable& selectable);

	// PlaneSelectable implementation
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) override;
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) override;

	// Snappable implementation
	virtual void snapto(float snap) override;

	// ComponentSnappable implementation
	void snapComponents(float snap) override;

	// Translatable implementation
	virtual void translate(const Vector3& translation) override;

	// Allocates a new node on the heap (via copy construction)
	scene::INodePtr clone() const override;

	// BrushObserver implementation
	void clear() override;
	void reserve(std::size_t size) override;
	void push_back(Face& face) override;
	void pop_back() override;
	void erase(std::size_t index) override;
	void connectivityChanged() override;
	void edge_clear() override;
	void edge_push_back(SelectableEdge& edge) override;
	void vertex_clear() override;
	void vertex_push_back(SelectableVertex& vertex) override;
	void DEBUG_verify() override;

	// Renderable implementation
    void onPreRender(const VolumeTest& volume) override;
	void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;
	std::size_t getHighlightFlags() override;
    void onFaceNeedsRenderableUpdate();

	void evaluateTransform();

	// Traceable implementation
	bool getIntersection(const Ray& ray, Vector3& intersection) override;

	// Update call, issued by the FilterSystem on potential shader visibility changes
	void updateFaceVisibility();

	void setClipPlane(const Plane3& plane);

	void forEachFaceInstance(const std::function<void(FaceInstance&)>& functor);

    // Returns the center of the untransformed world AABB
    const Vector3& getUntransformedOrigin() override;

    // Returns true if this node is visible due to its selection status
    // even though it might otherwise be filtered or hidden
    // Should only be used by the internal Brush object
    bool facesAreForcedVisible();

    void onPostUndo() override;
    void onPostRedo() override;

protected:
    virtual void onVisibilityChanged(bool isVisibleNow) override;

	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
	void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

    void onSelectionStatusChange(bool changeGroupStatus) override;

private:
	void transformComponents(const Matrix4& matrix);

	void updateSelectedPointsArray();

};
typedef std::shared_ptr<BrushNode> BrushNodePtr;
