#include "PatchNode.h"

#include "PatchInstance.h"

// Construct a PatchNode with no arguments
PatchNode::PatchNode(bool patchDef3) :
	m_patch(*this, 
			InstanceSetEvaluateTransform<PatchInstance>::Caller(m_instances), 
			InstanceSet::BoundsChangedCaller(m_instances)), // create the m_patch member with the node parameters
	m_importMap(m_patch),
	m_exportMap(m_patch)
{
	m_patch.m_patchDef3 = patchDef3;
}
  
// Copy Constructor
PatchNode::PatchNode(const PatchNode& other) :
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
{}

std::string PatchNode::name() const {
	return "Patch";
}

Patch& PatchNode::getPatch() {
	return m_patch;
}

// Snappable implementation
void PatchNode::snapto(float snap) {
	m_patch.snapto(snap);
}

// TransformNode implementation
const Matrix4& PatchNode::localToParent() const {
	return m_patch.localToParent();
}	
  
  	// MapImporter implementation
bool PatchNode::importTokens(Tokeniser& tokeniser) {
	return m_importMap.importTokens(tokeniser);
}

// MapExporter implementation
void PatchNode::exportTokens(std::ostream& os) const {
	m_exportMap.exportTokens(os);
}

// Clones this node, allocates a new Node on the heap and passes itself to the constructor of the new node
scene::INodePtr PatchNode::clone() const {
	return scene::INodePtr(new PatchNode(*this));
}

// This creates a new PatchInstance at the given scenepath and the given parent 
scene::Instance* PatchNode::create(const scene::Path& path, scene::Instance* parent) {
	// Create a new PatchInstance on the heap and return it
	return new PatchInstance(path, parent, m_patch);
}

// Cycles through all the instances of this node with the given Visitor class
void PatchNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

// Inserts a new instance
void PatchNode::insert(const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(path, instance);
}

// Removes an instance from the internal list
scene::Instance* PatchNode::erase(const scene::Path& path) {
	return m_instances.erase(path);
}
