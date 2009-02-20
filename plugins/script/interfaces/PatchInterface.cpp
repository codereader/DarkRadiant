#include "PatchInterface.h"

#include "ipatch.h"

namespace script {

class ScriptPatchNode :
	public ScriptSceneNode
{
public:
	ScriptPatchNode(const scene::INodePtr& node) :
		ScriptSceneNode((node != NULL && Node_isPatch(node)) ? node : scene::INodePtr())
	{}

	// Checks if the given SceneNode structure is a PatchNode
	static bool isPatch(const ScriptSceneNode& node) {
		return Node_isPatch(node);
	}

	// "Cast" service for Python, returns a ScriptPatchNode. 
	// The returned node is non-NULL if the cast succeeded
	static ScriptPatchNode getPatch(const ScriptSceneNode& node) {
		// Try to cast the node onto a patch
		IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(
			static_cast<scene::INodePtr>(node)
		);
		
		// Construct a patchNode (contained node may be NULL)
		return (patchNode != NULL) ? ScriptPatchNode(node) : ScriptPatchNode(scene::INodePtr());
	}
};

ScriptSceneNode PatchInterface::createPatchDef2() {
	// Create a new patch and return the script scene node
	return ScriptSceneNode(GlobalPatchCreator(DEF2).createPatch());
}

ScriptSceneNode PatchInterface::createPatchDef3() {
	// Create a new patch and return the script scene node
	return ScriptSceneNode(GlobalPatchCreator(DEF3).createPatch());
}

void PatchInterface::registerInterface(boost::python::object& nspace) {
	// Define a PatchNode interface
	nspace["PatchNode"] = boost::python::class_<ScriptPatchNode, 
		boost::python::bases<ScriptSceneNode> >("PatchNode", boost::python::init<const scene::INodePtr&>() )
		//.def("getNumFaces", &ScriptPatchNode::getNumFaces)
	;

	// Add the "isPatch" and "getPatch" method to all ScriptSceneNodes
	boost::python::object sceneNode = nspace["SceneNode"];

	boost::python::objects::add_to_namespace(sceneNode, 
		"isPatch", boost::python::make_function(&ScriptPatchNode::isPatch));

	boost::python::objects::add_to_namespace(sceneNode, 
		"getPatch", boost::python::make_function(&ScriptPatchNode::getPatch));
	
	// Define the GlobalPatchCreator interface
	nspace["GlobalPatchCreator"] = boost::python::class_<PatchInterface>("GlobalPatchCreator")
		.def("createPatchDef2", &PatchInterface::createPatchDef2)
		.def("createPatchDef3", &PatchInterface::createPatchDef3)
	;

	// Now point the Python variable "GlobalPatchCreator" to this instance
	nspace["GlobalPatchCreator"] = boost::python::ptr(this);
}

} // namespace script
