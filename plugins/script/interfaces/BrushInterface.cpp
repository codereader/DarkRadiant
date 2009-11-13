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
		scene::INodePtr node = _node.lock();
		if (node == NULL) return 0;

		IBrush* brush = Node_getIBrush(node);

		return (brush != NULL) ? brush->getNumFaces() : 0;
	}

	// Get a reference to the face by index in [0..getNumFaces).
	/*IFace& getFace(std::size_t index)
	{
	}*/

	// Returns true when this brush has no faces
	bool empty() const
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return true;

		return brushNode->getIBrush().empty();
	}

	// Returns true if any face of the brush contributes to the final B-Rep.
	bool hasContributingFaces() const
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return true;

		return brushNode->getIBrush().hasContributingFaces();
	}

	// Removes faces that do not contribute to the brush. 
	// This is useful for cleaning up after CSG operations on the brush.
	// Note: removal of empty faces is not performed during direct brush manipulations, 
	// because it would make a manipulation irreversible if it created an empty face.
	void removeEmptyFaces()
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return;

		brushNode->getIBrush().removeEmptyFaces();
	}

	// Sets the shader of all faces to the given name
	void setShader(const std::string& newShader)
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return;

		brushNode->getIBrush().setShader(newShader);
	}

	// Returns TRUE if any of the faces has the given shader
	bool hasShader(const std::string& name)
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return false;

		return brushNode->getIBrush().hasShader(name);
	}

	// Saves the current state to the undo stack.
	// Call this before manipulating the brush to make your action undo-able.
	void undoSave() 
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return;

		brushNode->getIBrush().undoSave();
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
			static_cast<scene::INodePtr>(node)
		);
		
		// Construct a brushnode (contained node may be NULL)
		return (brushNode != NULL) ? ScriptBrushNode(node) : ScriptBrushNode(scene::INodePtr());
	}
};

class ScriptFace
{
	IFace* _face;
	static std::string _emptyShader;
public:
	ScriptFace() :
		_face(NULL)
	{}

	ScriptFace(IFace& face) :
		_face(&face)
	{}

	void undoSave()
	{
		if (_face == NULL) return;
		_face->undoSave();
	}

	const std::string& getShader() const 
	{
		if (_face == NULL) return _emptyShader;
		return _face->getShader();
	}

	void setShader(const std::string& name) 
	{
		if (_face == NULL) return;
		_face->setShader(name);
	}

	void shiftTexdef(float s, float t) 
	{
		if (_face == NULL) return;
		_face->shiftTexdef(s, t);
	}

	void scaleTexdef(float s, float t) 
	{
		if (_face == NULL) return;
		_face->scaleTexdef(s, t);
	}

	void rotateTexdef(float angle) 
	{
		if (_face == NULL) return;
		_face->rotateTexdef(angle);
	}

	void fitTexture(float s_repeat, float t_repeat)
	{
		if (_face == NULL) return;
		_face->fitTexture(s_repeat, t_repeat);
	}

	void flipTexture(unsigned int flipAxis) 
	{
		if (_face == NULL) return;
		_face->flipTexture(flipAxis);
	}
	
	void normaliseTexture() 
	{
		if (_face == NULL) return;
		_face->normaliseTexture();
	}

	/*IWinding& getWinding() 
	{
		if (_face == NULL) return;
		_face->setShader(name);
	}*/
};

std::string ScriptFace::_emptyShader;

ScriptSceneNode BrushInterface::createBrush() {
	// Create a new brush and return the script scene node
	return ScriptSceneNode(GlobalBrushCreator().createBrush());
}

void BrushInterface::registerInterface(boost::python::object& nspace)
{
	// Define a "Face" interface
	nspace["Face"] = boost::python::class_<ScriptFace>("Face", boost::python::init<>())
		.def(boost::python::init<IFace&>())
		.def("undoSave", &ScriptFace::undoSave)
		.def("getShader", &ScriptFace::getShader, 
			boost::python::return_value_policy<boost::python::copy_const_reference>())
		.def("setShader", &IFace::setShader)
		.def("shiftTexdef", &IFace::shiftTexdef)
		.def("scaleTexdef", &IFace::scaleTexdef)
		.def("rotateTexdef", &IFace::rotateTexdef)
		.def("fitTexture", &IFace::fitTexture)
		.def("flipTexture", &IFace::flipTexture)
		.def("normaliseTexture", &IFace::normaliseTexture)
//		.def("getWinding", &IFace::getWinding)
		// TODO: getWinding
	;

	// Define a BrushNode interface
	nspace["BrushNode"] = boost::python::class_<ScriptBrushNode, 
		boost::python::bases<ScriptSceneNode> >("BrushNode", boost::python::init<const scene::INodePtr&>() )
		.def("getNumFaces", &ScriptBrushNode::getNumFaces)
		.def("empty", &ScriptBrushNode::empty)
		.def("hasContributingFaces", &ScriptBrushNode::hasContributingFaces)
		.def("removeEmptyFaces", &ScriptBrushNode::getNumFaces)
		.def("setShader", &ScriptBrushNode::setShader)
		.def("hasShader", &ScriptBrushNode::hasShader)
		.def("undoSave", &ScriptBrushNode::undoSave)
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
