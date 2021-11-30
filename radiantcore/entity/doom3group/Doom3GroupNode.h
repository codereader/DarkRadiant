#pragma once

#include "igroupnode.h"
#include "icurve.h"
#include "irenderable.h"
#include "../OriginKey.h"
#include "../RotationKey.h"
#include "../NameKey.h"
#include "../curve/CurveEditInstance.h"
#include "../curve/CurveNURBS.h"
#include "../curve/CurveCatmullRom.h"
#include "../VertexInstance.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"
#include "../KeyObserverDelegate.h"
#include "render/RenderablePivot.h"

namespace entity
{

class Doom3GroupNode;
typedef std::shared_ptr<Doom3GroupNode> Doom3GroupNodePtr;

class Doom3GroupNode :
	public EntityNode,
	public scene::GroupNode,
	public Snappable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public CurveNode
{
	// ------------------------------------------------------------------------
    // Doom3Group members

	OriginKey m_originKey;
	Vector3 m_origin;

	// A separate origin for the renderable pivot points
	Vector3 m_nameOrigin;

	RotationKey m_rotationKey;
	RotationMatrix m_rotation;

	render::RenderablePivot m_renderOrigin;

	mutable AABB m_curveBounds;

	// The value of the "name" key for this Doom3Group.
	std::string m_name;

	// The value of the "model" key for this Doom3Group.
	std::string m_modelKey;

	// Flag to indicate this Doom3Group is a model (i.e. does not contain
	// brushes).
	bool m_isModel;

	CurveNURBS m_curveNURBS;
	std::size_t m_curveNURBSChanged;
	CurveCatmullRom m_curveCatmullRom;
	std::size_t m_curveCatmullRomChanged;

	// ------------------------------------------------------------------------
    // Doom3GroupNode members

	CurveEditInstance _nurbsEditInstance;
	CurveEditInstance _catmullRomEditInstance;
	mutable AABB m_aabb_component;

	VertexInstance _originInstance;

private:
	// Constructor
	Doom3GroupNode(const IEntityClassPtr& eclass);
	// Private copy constructor, is invoked by clone()
	Doom3GroupNode(const Doom3GroupNode& other);

public:
	static Doom3GroupNodePtr Create(const IEntityClassPtr& eclass);

	virtual ~Doom3GroupNode();

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

	// override scene::Inode::onRemoveFromScene to deselect the child components
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
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	void renderComponents(RenderableCollector& collector, const VolumeTest& volume) const override;

	void transformComponents(const Matrix4& matrix);

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;

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

private:
	void evaluateTransform();

    // ------------------------------------------------------------------------
    // Doom3Group methods

	void destroy();
	void setIsModel(bool newValue);
    void renderCommon(RenderableCollector& collector, const VolumeTest& volume) const;

	/** Determine if this Doom3Group is a model (func_static) or a
	 * brush-containing entity. If the "model" key is equal to the
	 * "name" key, then this is a brush-based entity, otherwise it is
	 * a model entity. The exception to this is for the "worldspawn"
	 * entity class, which is always a brush-based entity.
	 */
	void updateIsModel();

	Vector3& getOrigin();
	void testSelect(Selector& selector, SelectionTest& test, SelectionIntersection& best);
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
