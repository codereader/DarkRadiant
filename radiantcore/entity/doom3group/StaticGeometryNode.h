#pragma once

#include "igroupnode.h"
#include "icurve.h"
#include "irenderable.h"
#include "editable.h"
#include "../OriginKey.h"
#include "../RotationKey.h"
#include "../NameKey.h"
#include "../curve/CurveEditInstance.h"
#include "../curve/CurveNURBS.h"
#include "../curve/CurveCatmullRom.h"
#include "../curve/RenderableCurveVertices.h"
#include "../VertexInstance.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"
#include "../KeyObserverDelegate.h"
#include "render/RenderablePivot.h"
#include "RenderableVertex.h"

namespace entity
{

/**
 * @brief Container node for static geometry, either in the form of brushes and
 * patches, or a model loaded from a file.
 *
 * This node is used for "func_static" and similar entities which contain static
 * geometry. It is always used as a parent node, either of the brushes and
 * patches it contains, or a StaticModelNode containing a model file. It never
 * stores or renders any geometry itself.
 */
class StaticGeometryNode :
	public EntityNode,
	public scene::GroupNode,
	public Snappable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public CurveNode
{
	OriginKey m_originKey;
	Vector3 m_origin;

	RotationKey m_rotationKey;
	RotationMatrix m_rotation;

	render::RenderablePivot _renderOrigin;

	mutable AABB m_curveBounds;

	// The value of the "name" key for this Doom3Group.
	std::string m_name;

	// The value of the "model" key for this Doom3Group.
	std::string m_modelKey;

	// Flag to indicate this Doom3Group is a model (i.e. does not contain
	// brushes).
	bool m_isModel;

	CurveNURBS m_curveNURBS;
	CurveCatmullRom m_curveCatmullRom;

	CurveEditInstance _nurbsEditInstance;
	CurveEditInstance _catmullRomEditInstance;
	mutable AABB m_aabb_component;

	VertexInstance _originInstance;

    ShaderPtr _pivotShader;
    ShaderPtr _pointShader;

    RenderableCurveVertices _nurbsVertices;
    RenderableCurveVertices _catmullRomVertices;
    RenderableVertex _renderableOriginVertex;

private:
	// Constructor
	StaticGeometryNode(const IEntityClassPtr& eclass);
	// Private copy constructor, is invoked by clone()
	StaticGeometryNode(const StaticGeometryNode& other);

public:
    using Ptr = std::shared_ptr<StaticGeometryNode>;

	static StaticGeometryNode::Ptr Create(const IEntityClassPtr& eclass);

	virtual ~StaticGeometryNode();

	// CurveNode implementation
	virtual bool hasEmptyCurve() override;
	virtual void appendControlPoints(unsigned int numPoints) override;
	virtual void removeSelectedControlPoints() override;
	virtual void insertControlPointsAtSelected() override;
	virtual void convertCurveType() override;

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	/** greebo: Tests the contained Doom3Group for selection.
	 *
	 * Note: This can be successful in vertex mode only, func_statics do not use this method.
	 */
	void testSelect(Selector& selector, SelectionTest& test) override;

	// ComponentSelectionTestable implementation
	bool isSelectedComponents() const override;
	void setSelectedComponents(bool selected, selection::ComponentSelectionMode mode) override;
	void invertSelectedComponents(selection::ComponentSelectionMode mode) override;
	void testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode) override;

	virtual void onInsertIntoScene(scene::IMapRootNode& root) override;
	virtual void onRemoveFromScene(scene::IMapRootNode& root) override;

	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const override;

	// ComponentSnappable implementation
	void snapComponents(float snap) override;

	// Snappable implementation
	virtual void snapto(float snap) override;

	void selectionChangedComponent(const ISelectable& selectable);

	scene::INodePtr clone() const override;

	/** greebo: Call this right before map save to let the child
	 * brushes have their origin recalculated.
	 */
	void addOriginToChildren() override;
	void removeOriginFromChildren() override;

	// Renderable implementation
    void onPreRender(const VolumeTest& volume) override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	void transformComponents(const Matrix4& matrix);

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;

    const Vector3& getWorldPosition() const override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
    void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

	// Model Key changed signal
	void onModelKeyChanged(const std::string& value) override;

	// Override EntityNode::construct()
	virtual void construct() override;

    void onVisibilityChanged(bool isVisibleNow) override;

    void onSelectionStatusChange(bool changeGroupStatus) override;

private:
	void evaluateTransform();

    // ------------------------------------------------------------------------
    // Doom3Group methods

	void destroy();
	void setIsModel(bool newValue);

	/** Determine if this Doom3Group is a model (func_static) or a
	 * brush-containing entity. If the "model" key is equal to the
	 * "name" key, then this is a brush-based entity, otherwise it is
	 * a model entity. The exception to this is for the "worldspawn"
	 * entity class, which is always a brush-based entity.
	 */
	void updateIsModel();

	Vector3& getOrigin();
	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	void scale(const Vector3& scale);
	void revertTransformInternal();
	void freezeTransformInternal();

	// Translates the origin only (without the children)
	void translateOrigin(const Vector3& translation);
	// Snaps the origin to the grid
	void snapOrigin(float snap);

	void translateChildren(const Vector3& childTranslation);

	// Returns TRUE if this D3Group is a model
	bool isModel() const;

public:

	void nameChanged(const std::string& value);
	void modelChanged(const std::string& value);
	void updateTransform();
	void originChanged();
	void rotationChanged();
};

} // namespace
