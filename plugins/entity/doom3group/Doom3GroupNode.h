#ifndef DOOM3GROUPNODE_H_
#define DOOM3GROUPNODE_H_

#include "igroupnode.h"
#include "icurve.h"
#include "Doom3Group.h"
#include "nameable.h"
#include "scenelib.h"
#include "irenderable.h"
#include "../NameKey.h"
#include "../curve/CurveEditInstance.h"
#include "../VertexInstance.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"

namespace entity {

class Doom3GroupNode :
	public EntityNode,
	public scene::GroupNode,
	public Snappable,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public CurveNode
{
	friend class Doom3Group;

	// The contained Doom3Group class
	Doom3Group m_contained;

	CurveEditInstance m_curveNURBS;
	CurveEditInstance m_curveCatmullRom;
	mutable AABB m_aabb_component;
	
	VertexInstance _originInstance;

	// TRUE if the skin needs updating
	mutable bool _updateSkin;

	bool _instantiated;

	// Private copy constructor, is invoked by clone()
	Doom3GroupNode(const Doom3GroupNode& other);

public:
	Doom3GroupNode(const IEntityClassPtr& eclass);
	virtual ~Doom3GroupNode();
	
	void construct();

	// EntityNode implementation
	virtual void refreshModel();

	// CurveNode implementation
	virtual bool hasEmptyCurve();
	virtual void appendControlPoints(unsigned int numPoints);
	virtual void removeSelectedControlPoints();
	virtual void insertControlPointsAtSelected();
	virtual void convertCurveType();

	// Bounded implementation
	virtual const AABB& localAABB() const;

	/** greebo: Tests the contained Doom3Group for selection. 
	 * 
	 * Note: This can be successful in vertex mode only, func_statics do not use this method.
	 */
	void testSelect(Selector& selector, SelectionTest& test);

	// ComponentSelectionTestable implementation
	bool isSelectedComponents() const;
	void setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode);
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	// override scene::Inode::onRemoveFromScene to deselect the child components
	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const;

	// ComponentSnappable implementation
	void snapComponents(float snap);

	// Snappable implementation
	virtual void snapto(float snap);

	void selectionChangedComponent(const Selectable& selectable);
	typedef MemberCaller1<Doom3GroupNode, const Selectable&, &Doom3GroupNode::selectionChangedComponent> SelectionChangedComponentCaller;

	scene::INodePtr clone() const;

	/** greebo: Call this right before map save to let the child
	 * brushes have their origin recalculated. 
	 */
	void addOriginToChildren();
	void removeOriginFromChildren();

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderComponents(RenderableCollector& collector, const VolumeTest& volume) const;

	void skinChanged(const std::string& value);
	typedef MemberCaller1<Doom3GroupNode, const std::string&, &Doom3GroupNode::skinChanged> SkinChangedCaller;

	void transformComponents(const Matrix4& matrix);

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
	void _onTransformationChanged();

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
	void _applyTransformation();

private:
	void evaluateTransform();

}; // class Doom3GroupNode
typedef boost::shared_ptr<Doom3GroupNode> Doom3GroupNodePtr;

} // namespace entity

#endif /*DOOM3GROUPNODE_H_*/
