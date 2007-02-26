#ifndef DOOM3GROUPINSTANCE_H_
#define DOOM3GROUPINSTANCE_H_

#include "Doom3Group.h"
#include "../targetable.h"

namespace entity {

class Doom3GroupInstance :
	public TargetableInstance,
	public TransformModifier,
	public Renderable,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable
{
	class TypeCasts {
		InstanceTypeCastTable m_casts;
	public:
		TypeCasts() {
			m_casts = TargetableInstance::StaticTypeCasts::instance().get();
			InstanceContainedCast<Doom3GroupInstance, Bounded>::install(m_casts);
			InstanceStaticCast<Doom3GroupInstance, Renderable>::install(m_casts);
			InstanceStaticCast<Doom3GroupInstance, SelectionTestable>::install(m_casts);
			InstanceStaticCast<Doom3GroupInstance, ComponentSelectionTestable>::install(m_casts);
			InstanceStaticCast<Doom3GroupInstance, ComponentEditable>::install(m_casts);
			InstanceStaticCast<Doom3GroupInstance, ComponentSnappable>::install(m_casts);
			InstanceStaticCast<Doom3GroupInstance, Transformable>::install(m_casts);
			InstanceIdentityCast<Doom3GroupInstance>::install(m_casts);
		}
		InstanceTypeCastTable& get() {
			return m_casts;
		}
	};

	Doom3Group& m_contained;
	CurveEdit m_curveNURBS;
	CurveEdit m_curveCatmullRom;
	mutable AABB m_aabb_component;

public:

	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	STRING_CONSTANT(Name, "Doom3GroupInstance");

	Doom3GroupInstance(const scene::Path& path, scene::Instance* parent, Doom3Group& contained);
	~Doom3GroupInstance();

	Bounded& get(NullType<Bounded>);
	
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
