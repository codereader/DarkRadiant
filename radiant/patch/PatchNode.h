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
	public scene::Node,
	public scene::Instantiable,
	public scene::Cloneable,
	public Nameable,
	public Snappable,
	public TransformNode,
	public MapImporter,
	public MapExporter,
	public IPatchNode
{
	typedef PatchNode<TokenImporter, TokenExporter> Self;

	InstanceSet m_instances;
	Patch m_patch;
	TokenImporter m_importMap;
	TokenExporter m_exportMap;

public:
	std::string name() const {
		return "Patch";
	}

	virtual Patch& getPatch() {
		return m_patch;
	}

	// Snappable implementation
	virtual void snapto(float snap) {
		m_patch.snapto(snap);
	}

	// TransformNode implementation
	virtual const Matrix4& localToParent() const {
		return m_patch.localToParent();
	}	
  
  	// MapImporter implementation
	virtual bool importTokens(Tokeniser& tokeniser) {
		return m_importMap.importTokens(tokeniser);
	}
	
	// MapExporter implementation
	virtual void exportTokens(std::ostream& os) const {
		m_exportMap.exportTokens(os);
	}
	
	// Construct a PatchNode with no arguments
	PatchNode(bool patchDef3 = false) :
		m_patch(*this, InstanceSetEvaluateTransform<PatchInstance>::Caller(m_instances), 
				InstanceSet::BoundsChangedCaller(m_instances)), // create the m_patch member with the node parameters
		m_importMap(m_patch),
		m_exportMap(m_patch)
	{
		m_patch.m_patchDef3 = patchDef3;
	}
  
	// Copy Constructor
	PatchNode(const PatchNode& other) :
		scene::Node(other),
		scene::Instantiable(other),
		scene::Cloneable(other),
		Nameable(other),
		Snappable(other),
		TransformNode(other),
		MapImporter(other),
		MapExporter(other),
		IPatchNode(other),
		m_patch(other.m_patch, *this, InstanceSetEvaluateTransform<PatchInstance>::Caller(m_instances), 
			    InstanceSet::BoundsChangedCaller(m_instances)), // create the patch out of the <other> one
		m_importMap(m_patch),
		m_exportMap(m_patch)
	{
	}
	
	// returns the Patch
	Patch& get() {
		return m_patch;
	}
	const Patch& get() const {
		return m_patch;
	}

	// Clones this node, allocates a new Node on the heap and passes itself to the constructor of the new node
	scene::Node& clone() const {
		return *(new PatchNode(*this));
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
