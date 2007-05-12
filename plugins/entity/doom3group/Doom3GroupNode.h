#ifndef DOOM3GROUPNODE_H_
#define DOOM3GROUPNODE_H_

#include "Doom3Group.h"
#include "instancelib.h"
#include "scenelib.h"
#include "../namedentity.h"

namespace entity {

class Doom3GroupNode :
	public scene::Node::Symbiot,
	public scene::Instantiable,
	public scene::Cloneable,
	public scene::Traversable::Observer,
	public scene::GroupNode
{
	class TypeCasts {
		NodeTypeCastTable m_casts;
	public:
		TypeCasts() {
			NodeStaticCast<Doom3GroupNode, scene::Instantiable>::install(m_casts);
			NodeStaticCast<Doom3GroupNode, scene::Cloneable>::install(m_casts);
			NodeStaticCast<Doom3GroupNode, scene::GroupNode>::install(m_casts);
			NodeContainedCast<Doom3GroupNode, scene::Traversable>::install(m_casts);
			NodeContainedCast<Doom3GroupNode, Snappable>::install(m_casts);
			NodeContainedCast<Doom3GroupNode, TransformNode>::install(m_casts);
			NodeContainedCast<Doom3GroupNode, Entity>::install(m_casts);
			NodeContainedCast<Doom3GroupNode, Namespaced>::install(m_casts);
			NodeContainedCast<Doom3GroupNode, ModelSkin>::install(m_casts);
			NodeContainedCast<Doom3GroupNode, Nameable>::install(m_casts);
		}
		NodeTypeCastTable& get() {
			return m_casts;
		}
	};

	// The actual node
	scene::Node m_node;
	
	// The Child instances of this node
	InstanceSet m_instances;
	
	// The contained Doom3Group class
	Doom3Group m_contained;

public:

	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	Doom3GroupNode(IEntityClassPtr eclass);
	Doom3GroupNode(const Doom3GroupNode& other);
	
	~Doom3GroupNode();

	// Casts
	scene::Traversable& get(NullType<scene::Traversable>);
	Snappable& get(NullType<Snappable>);
	TransformNode& get(NullType<TransformNode>);
	Entity& get(NullType<Entity>);
	Namespaced& get(NullType<Namespaced>);
	ModelSkin& get(NullType<ModelSkin>);
	Nameable& get(NullType<Nameable>);
	scene::GroupNode& get(NullType<scene::GroupNode>);

	scene::Node& node();

	scene::Node& clone() const;

	void insert(scene::Node& child);
	
	void erase(scene::Node& child);

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	
	void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance);
	
	scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path);

	/** greebo: Call this right before map save to let the child
	 * brushes have their origin recalculated. 
	 */
	void addOriginToChildren();
	void removeOriginFromChildren();

private:
	void construct();
	void destroy();

}; // class Doom3GroupNode

} // namespace entity

#endif /*DOOM3GROUPNODE_H_*/
