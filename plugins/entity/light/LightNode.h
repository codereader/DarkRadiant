#ifndef LIGHTNODE_H_
#define LIGHTNODE_H_

#include "scenelib.h"
#include "instancelib.h"

#include "Light.h"
#include "LightInstance.h"

class LightNode :
	public scene::Node,
	public scene::Instantiable,
	public scene::Cloneable,
	public scene::Traversable::Observer,
	public Nameable
{
	class TypeCasts {
		NodeTypeCastTable m_casts;
	public:
    	TypeCasts() {
			if(g_lightType == LIGHTTYPE_DOOM3) {
				NodeContainedCast<LightNode, scene::Traversable>::install(m_casts);
      		}
      		
			NodeContainedCast<LightNode, Editable>::install(m_casts);
			NodeContainedCast<LightNode, Snappable>::install(m_casts);
			NodeContainedCast<LightNode, TransformNode>::install(m_casts);
			NodeContainedCast<LightNode, Entity>::install(m_casts);
			NodeContainedCast<LightNode, Namespaced>::install(m_casts);
		}
		
		NodeTypeCastTable& get() {
			return m_casts;
		}
	}; // class TypeCasts

	InstanceSet m_instances;
	Light m_contained;

	void construct();	
	void destroy();
	
public:
	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	scene::Traversable& get(NullType<scene::Traversable>);
	Editable& get(NullType<Editable>);
	Snappable& get(NullType<Snappable>);
	TransformNode& get(NullType<TransformNode>);
	Entity& get(NullType<Entity>);
	Nameable& get(NullType<Nameable>);
	Namespaced& get(NullType<Namespaced>);

	LightNode(IEntityClassPtr eclass) :
		scene::Node(this, StaticTypeCasts::instance().get()),
		m_contained(eclass, *this, InstanceSet::TransformChangedCaller(m_instances), InstanceSet::BoundsChangedCaller(m_instances), InstanceSetEvaluateTransform<LightInstance>::Caller(m_instances))
	{
		construct();
	}
	
	LightNode(const LightNode& other) :
		scene::Node(this, StaticTypeCasts::instance().get()),
		scene::Instantiable(other),
		scene::Cloneable(other),
		scene::Traversable::Observer(other),
		Nameable(other),
		m_contained(other.m_contained, *this, InstanceSet::TransformChangedCaller(m_instances), InstanceSet::BoundsChangedCaller(m_instances), InstanceSetEvaluateTransform<LightInstance>::Caller(m_instances))
	{
		construct();
	}
	
	~LightNode() {
		destroy();
	}

	scene::Node& node();

	scene::Node& clone() const;

	void insert(scene::Node& child);
	void erase(scene::Node& child);

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance);
	scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path);
	
	// Nameable implementation
	virtual std::string name() const;
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);
}; // class LightNode

#endif /*LIGHTNODE_H_*/
