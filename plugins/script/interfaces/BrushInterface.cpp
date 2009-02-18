#include "BrushInterface.h"

#include "ibrush.h"

namespace script {

class ScriptBrushNode :
	public ScriptSceneNode
{
public:
	ScriptBrushNode(const scene::INodePtr& node) :
		ScriptSceneNode((node != NULL && Node_isBrush(node)) ? node : scene::INodePtr())
	{}

	std::size_t getNumFaces() {
		// Sanity check
		if (_node == NULL) return 0;

		IBrush* brush = Node_getIBrush(_node);

		return (brush != NULL) ? brush->size() : 0;
	}

	// Checks if the given SceneNode structure is a BrushNode
	static bool isBrush(const ScriptSceneNode& node) {
		return Node_isBrush(node);
	}

	// "Cast" service for Python, returns a ScriptBrushNode. 
	// The returned node is non-NULL if the cast succeeded
	static ScriptBrushNode getBrush(const ScriptSceneNode& node) {
		// Try to cast the node onto a brush
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(
			static_cast<const scene::INodePtr&>(node)
		);
		
		// Construct a brushnode (contained node may be NULL)
		return (brushNode != NULL) ? ScriptBrushNode(node) : ScriptBrushNode(scene::INodePtr());
	}
};

ScriptSceneNode BrushInterface::createBrush() {
	// Create a new brush and return the script scene node
	return ScriptSceneNode(GlobalBrushCreator().createBrush());
}

void BrushInterface::registerInterface(boost::python::object& nspace) {
	// Define a BrushNode interface
	nspace["BrushNode"] = boost::python::class_<ScriptBrushNode, 
		boost::python::bases<ScriptSceneNode> >("BrushNode", boost::python::init<const scene::INodePtr&>() )
		.def("getNumFaces", &ScriptBrushNode::getNumFaces)
	;

	// Add the "isBrush" and "getBrush" method to all ScriptSceneNodes
	boost::python::object sceneNode = nspace["SceneNode"];

	boost::python::objects::add_to_namespace(sceneNode, 
		"isBrush", boost::python::make_function(&ScriptBrushNode::isBrush));

	boost::python::objects::add_to_namespace(sceneNode, 
		"getBrush", boost::python::make_function(&ScriptBrushNode::getBrush));
	
	// Define the BrushCreator interface
	nspace["GlobalBrushCreator"] = boost::python::class_<BrushInterface>("GlobalBrushCreator")
		.def("createBrush", &BrushInterface::createBrush)
	;

	// Now point the Python variable "GlobalBrushCreator" to this instance
	nspace["GlobalBrushCreator"] = boost::python::ptr(this);
}

} // namespace script
