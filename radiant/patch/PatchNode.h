#ifndef PATCHNODE_H_
#define PATCHNODE_H_

#include "instancelib.h"
#include "scenelib.h"
#include "iscenegraph.h"
#include "imap.h"
#include "Patch.h"
#include "PatchInstance.h"
#include "PatchImportExport.h"

template<typename TokenImporter, typename TokenExporter>
class PatchNode :
	public scene::Node::Symbiot,
	public scene::Instantiable,
	public scene::Cloneable,
	public Nameable
{
	typedef PatchNode<TokenImporter, TokenExporter> Self;

	class TypeCasts	{
		InstanceTypeCastTable m_casts;
	public:
		// Constructor
		TypeCasts() {
			NodeStaticCast<PatchNode, scene::Instantiable>::install(m_casts);
			NodeStaticCast<PatchNode, scene::Cloneable>::install(m_casts);
			NodeStaticCast<PatchNode, Nameable>::install(m_casts);
			NodeContainedCast<PatchNode, Snappable>::install(m_casts);
			NodeContainedCast<PatchNode, TransformNode>::install(m_casts);
			NodeContainedCast<PatchNode, Patch>::install(m_casts);
			NodeContainedCast<PatchNode, MapImporter>::install(m_casts);
			NodeContainedCast<PatchNode, MapExporter>::install(m_casts);
		}
    	
    	InstanceTypeCastTable& get() {
			return m_casts;
		}
	}; // class Typecasts

	scene::Node m_node;
	InstanceSet m_instances;
	Patch m_patch;
	TokenImporter m_importMap;
	TokenExporter m_exportMap;

public:

	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	std::string name() const {
		return "Patch";
	}

	// Unused attach/detach functions (needed for nameable implementation)
	void attach(const NameCallback& callback) {}
	void detach(const NameCallback& callback) {}

	// Get the Snappable or TransformNode of the patch
	Snappable& get(NullType<Snappable>) {
		return m_patch;
	}	
	TransformNode& get(NullType<TransformNode>) {
		return m_patch;
	}	
  
	// "Casts" to MapImporter/MapExporter
	MapImporter& get(NullType<MapImporter>) {
		return m_importMap;
	}
	MapExporter& get(NullType<MapExporter>) {
		return m_exportMap;
	}
	
	// Construct a PatchNode with no arguments
	PatchNode(bool patchDef3 = false) :
		m_node(this, this, StaticTypeCasts::instance().get()),
		m_patch(m_node, InstanceSetEvaluateTransform<PatchInstance>::Caller(m_instances), 
				InstanceSet::BoundsChangedCaller(m_instances)), // create the m_patch member with the node parameters
		m_importMap(m_patch),
		m_exportMap(m_patch)
	{
		m_patch.m_patchDef3 = patchDef3;
	}
  
	// Copy Constructor
	PatchNode(const PatchNode& other) :
		scene::Node::Symbiot(other),
		scene::Instantiable(other),
		scene::Cloneable(other),
		Nameable(other),
		m_node(this, this, StaticTypeCasts::instance().get()),
		m_patch(other.m_patch, m_node, InstanceSetEvaluateTransform<PatchInstance>::Caller(m_instances), 
			    InstanceSet::BoundsChangedCaller(m_instances)), // create the patch out of the <other> one
		m_importMap(m_patch),
		m_exportMap(m_patch)
	{
	}
	
	// Get the actual scene::Node of this class 
	scene::Node& node() {
		return m_node;
	}
	
	// returns the Patch
	Patch& get() {
		return m_patch;
	}
	const Patch& get() const {
		return m_patch;
	}
	// "Casts" the Patch
	Patch& get(NullType<Patch>) {
		return m_patch;
	}

	// Clones this node, allocates a new Node on the heap and passes itself to the constructor of the new node
	scene::Node& clone() const {
		return (new PatchNode(*this))->node();
	}

	// This creates a new PatchInstance at the given scenepath and the given parent 
	scene::Instance* create(const scene::Path& path, scene::Instance* parent) {
		// Create a new PatchInstance on the heap and return it
		return new PatchInstance(path, parent, m_patch);
	}
	
	// Cycles through all the instances of this node with the given Visitor class
	void forEachInstance(const scene::Instantiable::Visitor& visitor) {
		m_instances.forEachInstance(visitor);
	}
	
	// Inserts a new instance
	void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance) {
		m_instances.insert(observer, path, instance);
	}
	
	// Removes an instance from the internal list
	scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path) {
		return m_instances.erase(observer, path);
	}
};

// A Doom 3 Patch Node
typedef PatchNode<PatchDoom3TokenImporter, PatchDoom3TokenExporter> PatchNodeDoom3;

#endif /*PATCHNODE_H_*/
