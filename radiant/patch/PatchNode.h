#ifndef PATCHNODE_H_
#define PATCHNODE_H_

#include "instancelib.h"
#include "scenelib.h"
#include "iscenegraph.h"
#include "imap.h"
#include "Patch.h"
#include "PatchImportExport.h"

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
	InstanceSet m_instances;
	Patch m_patch;
	PatchDoom3TokenImporter m_importMap;
	PatchDoom3TokenExporter m_exportMap;

public:
	// Construct a PatchNode with no arguments
	PatchNode(bool patchDef3 = false);
  
	// Copy Constructor
	PatchNode(const PatchNode& other);
	
	// Nameable implementation
	std::string name() const;

	// IPatchNode implementation
	virtual Patch& getPatch();

	// Snappable implementation
	virtual void snapto(float snap);

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;
  
  	// MapImporter implementation
	virtual bool importTokens(parser::DefTokeniser& tokeniser);
	
	// MapExporter implementation
	virtual void exportTokens(std::ostream& os) const;
	
	// Clones this node, allocates a new Node on the heap and passes itself to the constructor of the new node
	scene::INodePtr clone() const;

	// This creates a new PatchInstance at the given scenepath and the given parent 
	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	
	// Cycles through all the instances of this node with the given Visitor class
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	
	// Inserts a new instance
	void insert(const scene::Path& path, scene::Instance* instance);
	
	// Removes an instance from the internal list
	scene::Instance* erase(const scene::Path& path);
};

#endif /*PATCHNODE_H_*/
