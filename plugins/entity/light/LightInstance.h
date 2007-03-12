#ifndef LIGHTINSTANCE_H_
#define LIGHTINSTANCE_H_

#include "math/Vector3.h"
#include "math/matrix.h"
#include "selectionlib.h"
#include "dragplanes.h"

#include "Light.h"
#include "VertexInstance.h"
#include "../targetable.h"

/* greebo: The LightInstance class is some kind of manager for the Light class, and this is the
 * class the RadiantSelectionSystem is communicating with.
 * 
 * All the selection tests and callbacks are coming in here, most of the calls are passed to the
 * actual light class, some need some preparation.
 * 
 * Note: The actual Light class is in turn owned by the LightNode, only a reference to the Light 
 * is stored in LightInstance::_light.    
 */
 
class LightInstance :
	public TargetableInstance,
	public TransformModifier,
	public Renderable,
	public SelectionTestable,
	public RendererLight,
	public PlaneSelectable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable
{
	class TypeCasts {
		InstanceTypeCastTable m_casts;
	public:
		TypeCasts() {
			m_casts = TargetableInstance::StaticTypeCasts::instance().get();
			InstanceContainedCast<LightInstance, Bounded>::install(m_casts);
			//InstanceContainedCast<LightInstance, Cullable>::install(m_casts);
			InstanceStaticCast<LightInstance, Renderable>::install(m_casts);
			InstanceStaticCast<LightInstance, SelectionTestable>::install(m_casts);
			InstanceStaticCast<LightInstance, Transformable>::install(m_casts);
			InstanceStaticCast<LightInstance, PlaneSelectable>::install(m_casts);
			InstanceStaticCast<LightInstance, ComponentSelectionTestable>::install(m_casts);
			InstanceStaticCast<LightInstance, ComponentSnappable>::install(m_casts);
			InstanceIdentityCast<LightInstance>::install(m_casts);
		}
		
		InstanceTypeCastTable& get() {
			return m_casts;
		}
	};

	// The reference to the light class (owned by LightNode)
	Light& _light;
	
	// The (draggable) light center instance
	VertexInstance _lightCenterInstance;
	
	VertexInstance _lightTargetInstance;
	VertexInstanceRelative _lightRightInstance;
	VertexInstanceRelative _lightUpInstance;
	VertexInstance _lightStartInstance;
	VertexInstance _lightEndInstance;
	
	// dragplanes for lightresizing using mousedrag
	DragPlanes m_dragPlanes;
	// a temporary variable for calculating the AABB of all (selected) components
	mutable AABB m_aabb_component;
	
public:
	STRING_CONSTANT(Name, "LightInstance");
	
	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	Bounded& get(NullType<Bounded>);

	LightInstance(const scene::Path& path, scene::Instance* parent, Light& contained);	
	~LightInstance();
	
	void renderInactiveComponents(Renderer& renderer, const VolumeTest& volume, const bool selected) const;	
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;  
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;
  
	// Renders the components of this light instance 
	void renderComponents(Renderer& renderer, const VolumeTest& volume) const;
	
	const AABB& getSelectedComponentsBounds() const;
  
	// Test the light volume for selection, this just passes the call on to the contained Light class
	void testSelect(Selector& selector, SelectionTest& test);

	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);
  
	// greebo: Returns true if drag planes or the light center is selected (both are components)
	bool isSelectedComponents() const;
	
	// greebo: Selects/deselects all components, depending on the chosen componentmode
	void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode);
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	// This gets called to snap the components to the grid
	void snapComponents(float snap);

	void selectedChangedComponent(const Selectable& selectable);
	typedef MemberCaller1<LightInstance, const Selectable&, &LightInstance::selectedChangedComponent> SelectedChangedComponentCaller;

	void evaluateTransform();
	void applyTransform();
	typedef MemberCaller<LightInstance, &LightInstance::applyTransform> ApplyTransformCaller;

	/*greebo: This is a callback function that gets connected in the constructor
	* Don't know exactly what it does, but it seems to notify the shader cache that the light has moved or
	* something like that.*/ 
	void lightChanged();
	typedef MemberCaller<LightInstance, &LightInstance::lightChanged> LightChangedCaller;

	ShaderPtr getShader() const;
	const AABB& aabb() const;
	bool testAABB(const AABB& other) const;
	const Matrix4& rotation() const;
	const Vector3& offset() const;
	const Vector3& colour() const;

	bool isProjected() const;
	const Matrix4& projection() const;
}; // class LightInstance

#endif /*LIGHTINSTANCE_H_*/
