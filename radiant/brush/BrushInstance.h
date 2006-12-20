#ifndef BRUSHINSTANCE_H_
#define BRUSHINSTANCE_H_

#include "renderable.h"
#include "irender.h"
#include "scenelib.h"
#include "selectable.h"

#include "BrushClass.h"
#include "FaceInstance.h"
#include "EdgeInstance.h"
#include "VertexInstance.h"
#include "BrushClipPlane.h"

class BrushInstanceVisitor {
public:
	virtual void visit(FaceInstance& face) const = 0;
};

class BrushInstance :
		public BrushObserver,
		public scene::Instance,
		public Selectable,
		public Renderable,
		public SelectionTestable,
		public ComponentSelectionTestable,
		public ComponentEditable,
		public ComponentSnappable,
		public PlaneSelectable,
		public LightCullable
{
	class TypeCasts {
		InstanceTypeCastTable m_casts;
	public:
		TypeCasts() {
			InstanceStaticCast<BrushInstance, Selectable>::install(m_casts);
			InstanceContainedCast<BrushInstance, Bounded>::install(m_casts);
			InstanceContainedCast<BrushInstance, Cullable>::install(m_casts);
			InstanceStaticCast<BrushInstance, Renderable>::install(m_casts);
			InstanceStaticCast<BrushInstance, SelectionTestable>::install(m_casts);
			InstanceStaticCast<BrushInstance, ComponentSelectionTestable>::install(m_casts);
			InstanceStaticCast<BrushInstance, ComponentEditable>::install(m_casts);
			InstanceStaticCast<BrushInstance, ComponentSnappable>::install(m_casts);
			InstanceStaticCast<BrushInstance, PlaneSelectable>::install(m_casts);
			InstanceIdentityCast<BrushInstance>::install(m_casts);
			InstanceContainedCast<BrushInstance, Transformable>::install(m_casts);
		}
		InstanceTypeCastTable& get() {
			return m_casts;
		}
	};


	Brush& m_brush;

	FaceInstances m_faceInstances;

	typedef std::vector<EdgeInstance> EdgeInstances;
	EdgeInstances m_edgeInstances;
	typedef std::vector<VertexInstance> VertexInstances;
	VertexInstances m_vertexInstances;

	ObservedSelectable m_selectable;

	mutable RenderableWireframe m_render_wireframe;
	mutable RenderablePointVector m_render_selected;
	mutable AABB m_aabb_component;
	mutable Array<PointVertex> m_faceCentroidPointsCulled;
	RenderablePointArray m_render_faces_wireframe;
	mutable bool m_viewChanged; // requires re-evaluation of view-dependent cached data

	BrushClipPlane m_clipPlane;

	static Shader* m_state_selpoint;

	const LightList* m_lightList;

	TransformModifier m_transform;

	BrushInstance(const BrushInstance& other); // NOT COPYABLE
	BrushInstance& operator=(const BrushInstance& other); // NOT ASSIGNABLE
public:
	static Counter* m_counter;

	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	void lightsChanged();
	typedef MemberCaller<BrushInstance, &BrushInstance::lightsChanged> LightsChangedCaller;

	STRING_CONSTANT(Name, "BrushInstance");

	BrushInstance(const scene::Path& path, scene::Instance* parent, Brush& brush);
	~BrushInstance();

	Brush& getBrush();
	const Brush& getBrush() const;

	Bounded& get(NullType<Bounded>);
	Cullable& get(NullType<Cullable>);
	Transformable& get(NullType<Transformable>);

	void selectedChanged(const Selectable& selectable);
	typedef MemberCaller1<BrushInstance, const Selectable&, &BrushInstance::selectedChanged> SelectedChangedCaller;

	void selectedChangedComponent(const Selectable& selectable);
	typedef MemberCaller1<BrushInstance, const Selectable&, &BrushInstance::selectedChangedComponent> SelectedChangedComponentCaller;

	const BrushInstanceVisitor& forEachFaceInstance(const BrushInstanceVisitor& visitor);

	static void constructStatic();
	static void destroyStatic();

	void clear();
	void reserve(std::size_t size);

	void push_back(Face& face);
	void pop_back();
	void erase(std::size_t index);
	void connectivityChanged();

	void edge_clear();
	void edge_push_back(SelectableEdge& edge);

	void vertex_clear();
	void vertex_push_back(SelectableVertex& vertex);

	void DEBUG_verify() const;

	bool isSelected() const;
	void setSelected(bool select);

	void update_selected() const;

	void evaluateViewDependent(const VolumeTest& volume, const Matrix4& localToWorld) const;

	void renderComponentsSelected(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderComponents(Renderer& renderer, const VolumeTest& volume) const;

	void renderClipPlane(Renderer& renderer, const VolumeTest& volume) const;

	void renderCommon(Renderer& renderer, const VolumeTest& volume) const;

	void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	void viewChanged() const;

	void testSelect(Selector& selector, SelectionTest& test);

	bool isSelectedComponents() const;
	void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode);
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);

	void transformComponents(const Matrix4& matrix);
	const AABB& getSelectedComponentsBounds() const;

	void snapComponents(float snap);
	void evaluateTransform();
	void applyTransform();
	typedef MemberCaller<BrushInstance, &BrushInstance::applyTransform> ApplyTransformCaller;

	void setClipPlane(const Plane3& plane);

	bool testLight(const RendererLight& light) const;
	void insertLight(const RendererLight& light);
	void clearLights();
};

inline BrushInstance* Instance_getBrush(scene::Instance& instance) {
	return InstanceTypeCast<BrushInstance>::cast(instance);
}

#endif /*BRUSHINSTANCE_H_*/
