#include "BrushInterface.h"

#include "../SceneNodeBuffer.h"
#include <pybind11/stl_bind.h>

PYBIND11_MAKE_OPAQUE(IWinding);

namespace script
{

ScriptFace::ScriptFace() :
	_face(NULL)
{}

ScriptFace::ScriptFace(IFace& face) :
	_face(&face)
{}

void ScriptFace::undoSave()
{
	if (_face == NULL) return;
	_face->undoSave();
}

const std::string& ScriptFace::getShader() const
{
	if (_face == NULL) return _emptyShader;
	return _face->getShader();
}

void ScriptFace::setShader(const std::string& name)
{
	if (_face == NULL) return;
	_face->setShader(name);
}

void ScriptFace::shiftTexdef(float s, float t)
{
	if (_face == NULL) return;
	_face->shiftTexdef(s, t);
}

void ScriptFace::scaleTexdef(float s, float t)
{
	if (_face == NULL) return;
	_face->scaleTexdef(s, t);
}

void ScriptFace::rotateTexdef(float angle)
{
	if (_face == NULL) return;
	_face->rotateTexdef(angle);
}

void ScriptFace::fitTexture(float s_repeat, float t_repeat)
{
	if (_face == NULL) return;
	_face->fitTexture(s_repeat, t_repeat);
}

void ScriptFace::flipTexture(unsigned int flipAxis)
{
	if (_face == NULL) return;
	_face->flipTexture(flipAxis);
}

void ScriptFace::normaliseTexture()
{
	if (_face == NULL) return;
	_face->normaliseTexture();
}

IWinding& ScriptFace::getWinding()
{
	if (_face == NULL) return _emptyWinding;
	return _face->getWinding();
}

std::string ScriptFace::_emptyShader;
IWinding ScriptFace::_emptyWinding;

ScriptBrushNode::ScriptBrushNode(const scene::INodePtr& node) :
	ScriptSceneNode((node != NULL && Node_isBrush(node)) ? node : scene::INodePtr())
{}

std::size_t ScriptBrushNode::getNumFaces() {
	// Sanity check
	scene::INodePtr node = _node.lock();
	if (node == NULL) return 0;

	IBrush* brush = Node_getIBrush(node);

	return (brush != NULL) ? brush->getNumFaces() : 0;
}

ScriptFace ScriptBrushNode::getFace(std::size_t index)
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return ScriptFace();

	IBrush& brush = brushNode->getIBrush();
	return (index < brush.getNumFaces()) ? ScriptFace(brush.getFace(index)) : ScriptFace();
}

bool ScriptBrushNode::empty() const
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return true;

	return brushNode->getIBrush().empty();
}

bool ScriptBrushNode::hasContributingFaces() const
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return true;

	return brushNode->getIBrush().hasContributingFaces();
}

void ScriptBrushNode::removeEmptyFaces()
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return;

	brushNode->getIBrush().removeEmptyFaces();
}

void ScriptBrushNode::setShader(const std::string& newShader)
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return;

	brushNode->getIBrush().setShader(newShader);
}

bool ScriptBrushNode::hasShader(const std::string& name)
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return false;

	return brushNode->getIBrush().hasShader(name);
}

bool ScriptBrushNode::hasVisibleMaterial()
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return false;

	return brushNode->getIBrush().hasVisibleMaterial();
}

ScriptBrushNode::DetailFlag ScriptBrushNode::getDetailFlag()
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return Structural;

	return static_cast<DetailFlag>(brushNode->getIBrush().getDetailFlag());
}

void ScriptBrushNode::setDetailFlag(DetailFlag detailFlag)
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return;

	brushNode->getIBrush().setDetailFlag(static_cast<IBrush::DetailFlag>(detailFlag));
}

void ScriptBrushNode::undoSave()
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(_node.lock());
	if (brushNode == NULL) return;

	brushNode->getIBrush().undoSave();
}

// Checks if the given SceneNode structure is a BrushNode
bool ScriptBrushNode::isBrush(const ScriptSceneNode& node) 
{
	return Node_isBrush(node);
}

ScriptBrushNode ScriptBrushNode::getBrush(const ScriptSceneNode& node)
{
	// Try to cast the node onto a brush
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(
		static_cast<scene::INodePtr>(node)
	);

	// Construct a brushnode (contained node may be NULL)
	return (brushNode != NULL) ? ScriptBrushNode(node) : ScriptBrushNode(scene::INodePtr());
}

ScriptSceneNode BrushInterface::createBrush()
{
	// Create a new brush and return the script scene node
	scene::INodePtr node = GlobalBrushCreator().createBrush();

	// Add the node to the buffer otherwise it will be deleted immediately,
	// as ScriptSceneNodes are using weak_ptrs.
	SceneNodeBuffer::Instance().push_back(node);

	return ScriptSceneNode(node);
}

void BrushInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Define a WindingVertex structure
	py::class_<WindingVertex> vertex(scope, "WindingVertex");
	vertex.def(py::init<>());
	vertex.def_readonly("vertex", &WindingVertex::vertex);
	vertex.def_readonly("texcoord", &WindingVertex::texcoord);
	vertex.def_readonly("tangent", &WindingVertex::tangent);
	vertex.def_readonly("bitangent", &WindingVertex::bitangent);
	vertex.def_readonly("normal", &WindingVertex::normal);
	vertex.def_readonly("adjacent", &WindingVertex::adjacent);
	
	// Declare the IWinding vector
	py::bind_vector<IWinding>(scope, "Winding");

	// Define a "Face" interface
	py::class_<ScriptFace> face(scope, "Face");
	face.def(py::init<>());
	face.def(py::init<IFace&>());
	face.def("undoSave", &ScriptFace::undoSave);
	face.def("getShader", &ScriptFace::getShader, py::return_value_policy::reference);
	face.def("setShader", &ScriptFace::setShader);
	face.def("shiftTexdef", &ScriptFace::shiftTexdef);
	face.def("scaleTexdef", &ScriptFace::scaleTexdef);
	face.def("rotateTexdef", &ScriptFace::rotateTexdef);
	face.def("fitTexture", &ScriptFace::fitTexture);
	face.def("flipTexture", &ScriptFace::flipTexture);
	face.def("normaliseTexture", &ScriptFace::normaliseTexture);
	face.def("getWinding", &ScriptFace::getWinding, py::return_value_policy::reference);

	// Define a BrushNode interface
	py::class_<ScriptBrushNode, ScriptSceneNode> brush(scope, "BrushNode");
	brush.def(py::init<const scene::INodePtr&>());
	brush.def("getNumFaces", &ScriptBrushNode::getNumFaces);
	brush.def("empty", &ScriptBrushNode::empty);
	brush.def("hasContributingFaces", &ScriptBrushNode::hasContributingFaces);
	brush.def("removeEmptyFaces", &ScriptBrushNode::getNumFaces);
	brush.def("setShader", &ScriptBrushNode::setShader);
	brush.def("hasShader", &ScriptBrushNode::hasShader);
	brush.def("hasVisibleMaterial", &ScriptBrushNode::hasVisibleMaterial);
	brush.def("undoSave", &ScriptBrushNode::undoSave);
	brush.def("getFace", &ScriptBrushNode::getFace);
	brush.def("getDetailFlag", &ScriptBrushNode::getDetailFlag);
	brush.def("setDetailFlag", &ScriptBrushNode::setDetailFlag);

	// Define the BrushCreator interface
	py::class_<BrushInterface> brushCreator(scope, "BrushCreator");
	brushCreator.def("createBrush", &BrushInterface::createBrush);

	// Now point the Python variable "GlobalBrushCreator" to this instance
	globals["GlobalBrushCreator"] = this;

	py::enum_<ScriptBrushNode::DetailFlag>(scope, "BrushDetailFlag")
		.value("Structural", ScriptBrushNode::Structural)
		.value("Detail", ScriptBrushNode::Detail)
		.export_values();
}

} // namespace script
