#ifndef DOOM3GROUPNODE_H_
#define DOOM3GROUPNODE_H_

#include "igroupnode.h"
#include "icurve.h"
#include "Doom3Group.h"
#include "nameable.h"
#include "scenelib.h"
#include "irenderable.h"
#include "../namedentity.h"
#include "../curve/CurveEditInstance.h"
#include "../VertexInstance.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"

namespace entity {

class Doom3GroupNode :
	public EntityNode,
	public SelectableNode,
	public scene::Cloneable,
	public scene::GroupNode,
	public Nameable,
	public Snappable,
	public TransformNode,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public Renderable,
	public Bounded,
	public TransformModifier,
	public CurveNode,
	public TargetableNode
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

	// Private copy constructor, is invoked by clone()
	Doom3GroupNode(const Doom3GroupNode& other);

public:
	Doom3GroupNode(IEntityClassPtr eclass);
	~Doom3GroupNode();
	
	// EntityNode implementation
	virtual Entity& getEntity();
	virtual void refreshModel();

	// CurveNode implementation
	virtual bool hasEmptyCurve();
	virtual void appendControlPoints(unsigned int numPoints);
	virtual void removeSelectedControlPoints();
	virtual void insertControlPointsAtSelected();
	virtual void convertCurveType();

	// Namespaced implementation
	//virtual void setNamespace(INamespace& space);

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

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
	virtual void onRemoveFromScene();

	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const;

	// ComponentSnappable implementation
	void snapComponents(float snap);

	// Nameable implementation
	virtual std::string name() const;
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);

	// Snappable implementation
	virtual void snapto(float snap);

	void selectionChangedComponent(const Selectable& selectable);
	typedef MemberCaller1<Doom3GroupNode, const Selectable&, &Doom3GroupNode::selectionChangedComponent> SelectionChangedComponentCaller;

	scene::INodePtr clone() const;

	// scene::Instantiable implementation
	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);

	/** greebo: Call this right before map save to let the child
	 * brushes have their origin recalculated. 
	 */
	void addOriginToChildren();
	void removeOriginFromChildren();

	// Renderable implementation
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;
	void renderComponents(Renderer& renderer, const VolumeTest& volume) const;

	void evaluateTransform();
	typedef MemberCaller<Doom3GroupNode, &Doom3GroupNode::evaluateTransform> EvaluateTransformCaller;

	void applyTransform();
	typedef MemberCaller<Doom3GroupNode, &Doom3GroupNode::applyTransform> ApplyTransformCaller;

	void skinChanged(const std::string& value);
	typedef MemberCaller1<Doom3GroupNode, const std::string&, &Doom3GroupNode::skinChanged> SkinChangedCaller;

	void transformComponents(const Matrix4& matrix);

private:
	void construct();

}; // class Doom3GroupNode

} // namespace entity

#endif /*DOOM3GROUPNODE_H_*/
