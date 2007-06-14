#ifndef DOOM3GROUPINSTANCE_H_
#define DOOM3GROUPINSTANCE_H_

#include "icurve.h"
#include "Doom3Group.h"
#include "../curve/CurveEditInstance.h"
#include "../VertexInstance.h"
#include "../targetable.h"

namespace entity {

class Doom3GroupInstance :
	public TargetableInstance,
	public TransformModifier,
	public Renderable,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public Bounded,
	public CurveInstance
{
	Doom3Group& m_contained;
	CurveEditInstance m_curveNURBS;
	CurveEditInstance m_curveCatmullRom;
	mutable AABB m_aabb_component;
	
	VertexInstance _originInstance;

public:
	STRING_CONSTANT(Name, "Doom3GroupInstance");

	Doom3GroupInstance(const scene::Path& path, scene::Instance* parent, Doom3Group& contained);
	~Doom3GroupInstance();

	// CurveInstance implementation
	virtual bool hasEmptyCurve();
	virtual void appendControlPoints(unsigned int numPoints);
	virtual void removeSelectedControlPoints();
	virtual void insertControlPointsAtSelected();

	// Bounded implementation
	virtual const AABB& localAABB() const;
	
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;
	void renderComponents(Renderer& renderer, const VolumeTest& volume) const;

	/** greebo: Tests the contained Doom3Group for selection. 
	 * 
	 * Note: This can be successful in vertex mode only, func_statics do not use this method.
	 */
	void testSelect(Selector& selector, SelectionTest& test);

	bool isSelectedComponents() const;
	void setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode);
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	void transformComponents(const Matrix4& matrix);

	const AABB& getSelectedComponentsBounds() const;

	void snapComponents(float snap);

	void evaluateTransform();
	void applyTransform();
	typedef MemberCaller<Doom3GroupInstance, &Doom3GroupInstance::applyTransform> ApplyTransformCaller;

	void selectionChangedComponent(const Selectable& selectable);
	typedef MemberCaller1<Doom3GroupInstance, const Selectable&, &Doom3GroupInstance::selectionChangedComponent> SelectionChangedComponentCaller;

}; // class Doom3GroupInstance

} // namespace entity

#endif /*DOOM3GROUPINSTANCE_H_*/
