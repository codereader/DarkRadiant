#include "BrushInterface.h"

#include "ibrush.h"
#include "../SceneNodeBuffer.h"
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace script {

class ScriptFace
{
	IFace* _face;
	static std::string _emptyShader;
	static IWinding _emptyWinding;

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

	IWinding& getWinding()
	{
		if (_face == NULL) return _emptyWinding;
		return _face->getWinding();
	}
};

std::string ScriptFace::_emptyShader;
IWinding ScriptFace::_emptyWinding;

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
	ScriptFace getFace(std::size_t index)
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return ScriptFace();

		IBrush& brush = brushNode->getIBrush();
		return (index < brush.getNumFaces()) ? ScriptFace(brush.getFace(index)) : ScriptFace();
	}

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

	bool hasVisibleMaterial()
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return false;

		return brushNode->getIBrush().hasVisibleMaterial();
	}

	enum DetailFlag
	{
		Structural = 0,
		Detail = 1 << 27,
	};

	DetailFlag getDetailFlag()
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return Structural;

		return static_cast<DetailFlag>(brushNode->getIBrush().getDetailFlag());
	}

	void setDetailFlag(DetailFlag detailFlag)
	{
		IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(_node.lock());
		if (brushNode == NULL) return;

		brushNode->getIBrush().setDetailFlag(static_cast<IBrush::DetailFlag>(detailFlag));
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

ScriptSceneNode BrushInterface::createBrush()
{
	// Create a new brush and return the script scene node
	scene::INodePtr node = GlobalBrushCreator().createBrush();

	// Add the node to the buffer otherwise it will be deleted immediately,
	// as ScriptSceneNodes are using weak_ptrs.
	SceneNodeBuffer::Instance().push_back(node);

	return ScriptSceneNode(node);
}

void BrushInterface::registerInterface(boost::python::object& nspace)
{
	// Define a WindingVertex structure
	nspace["WindingVertex"] = boost::python::class_<WindingVertex>("WindingVertex", boost::python::init<>())
		.def_readonly("vertex", &WindingVertex::vertex)
		.def_readonly("texcoord", &WindingVertex::texcoord)
		.def_readonly("tangent", &WindingVertex::tangent)
		.def_readonly("bitangent", &WindingVertex::bitangent)
		.def_readonly("normal", &WindingVertex::normal)
		.def_readonly("adjacent", &WindingVertex::adjacent)
	;

	// Declare the IWinding vector
	boost::python::class_<IWinding>("Winding")
		.def(boost::python::vector_indexing_suite<IWinding>())
	;

	// Define a "Face" interface
	nspace["Face"] = boost::python::class_<ScriptFace>("Face", boost::python::init<>())
		.def(boost::python::init<IFace&>())
		.def("undoSave", &ScriptFace::undoSave)
		.def("getShader", &ScriptFace::getShader,
			boost::python::return_value_policy<boost::python::copy_const_reference>())
		.def("setShader", &ScriptFace::setShader)
		.def("shiftTexdef", &ScriptFace::shiftTexdef)
		.def("scaleTexdef", &ScriptFace::scaleTexdef)
		.def("rotateTexdef", &ScriptFace::rotateTexdef)
		.def("fitTexture", &ScriptFace::fitTexture)
		.def("flipTexture", &ScriptFace::flipTexture)
		.def("normaliseTexture", &ScriptFace::normaliseTexture)
		.def("getWinding", &ScriptFace::getWinding,
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
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
		.def("hasVisibleMaterial", &ScriptBrushNode::hasVisibleMaterial)
		.def("undoSave", &ScriptBrushNode::undoSave)
		.def("getFace", &ScriptBrushNode::getFace)
		.def("getDetailFlag", &ScriptBrushNode::getDetailFlag)
		.def("setDetailFlag", &ScriptBrushNode::setDetailFlag)
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

	boost::python::scope in_class( // establish new scope for definitions
		nspace["BrushNode"]
	);

	nspace["BrushDetailFlag"] = boost::python::enum_<ScriptBrushNode::DetailFlag>("BrushDetailFlag")
		.value("Structural", ScriptBrushNode::Structural)
		.value("Detail", ScriptBrushNode::Detail)		
    ;
}

} // namespace script
