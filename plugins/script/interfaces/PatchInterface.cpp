#include "PatchInterface.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include "ipatch.h"
#include "itextstream.h"

#include "../SceneNodeBuffer.h"

namespace script 
{

ScriptPatchNode::ScriptPatchNode(const scene::INodePtr& node) :
	ScriptSceneNode((node != NULL && Node_isPatch(node)) ? node : scene::INodePtr())
{}

bool ScriptPatchNode::isPatch(const ScriptSceneNode& node)
{
	return Node_isPatch(node);
}

ScriptPatchNode ScriptPatchNode::getPatch(const ScriptSceneNode& node) 
{
	// Try to cast the node onto a patch
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(
		static_cast<scene::INodePtr>(node)
	);

	// Construct a patchNode (contained node may be NULL)
	return (patchNode != NULL) ? ScriptPatchNode(node) : ScriptPatchNode(scene::INodePtr());
}

void ScriptPatchNode::setDims(std::size_t width, std::size_t height)
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return;

	patchNode->getPatch().setDims(width, height);
}

std::size_t ScriptPatchNode::getWidth() const
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return 0;

	return patchNode->getPatch().getWidth();
}

std::size_t ScriptPatchNode::getHeight() const
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return 0;

	return patchNode->getPatch().getHeight();
}

PatchMesh ScriptPatchNode::getTesselatedPatchMesh() const
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return PatchMesh();

	return patchNode->getPatch().getTesselatedPatchMesh();
}

PatchControl& ScriptPatchNode::ctrlAt(std::size_t row, std::size_t col)
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return _emptyPatchControl;

	IPatch& patch = patchNode->getPatch();

	if (row > patch.getHeight() || col > patch.getWidth())
	{
		rError() << "One or more patch control indices out of bounds: " << row << "," << col << std::endl;
		return _emptyPatchControl;
	}

	return patchNode->getPatch().ctrlAt(row, col);
}

void ScriptPatchNode::insertColumns(std::size_t colIndex)
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return;

	return patchNode->getPatch().insertColumns(colIndex);
}

void ScriptPatchNode::insertRows(std::size_t rowIndex)
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return;

	return patchNode->getPatch().insertRows(rowIndex);
}

void ScriptPatchNode::removePoints(int columns, std::size_t index)
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return;

	return patchNode->getPatch().removePoints(static_cast<bool>(columns), index);
}

void ScriptPatchNode::appendPoints(int columns, int beginning)
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return;

	return patchNode->getPatch().appendPoints(static_cast<bool>(columns), static_cast<bool>(beginning));
}

bool ScriptPatchNode::isValid() const
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return false;

	return patchNode->getPatch().isValid();
}

bool ScriptPatchNode::isDegenerate() const
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return true;

	return patchNode->getPatch().isDegenerate();
}

void ScriptPatchNode::controlPointsChanged()
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return;

	patchNode->getPatch().controlPointsChanged();
}

const std::string& ScriptPatchNode::getShader() const
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return _emptyShader;

	return patchNode->getPatch().getShader();
}

void ScriptPatchNode::setShader(const std::string& name)
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return;

	patchNode->getPatch().setShader(name);
}

bool ScriptPatchNode::hasVisibleMaterial()
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return false;

	return patchNode->getPatch().hasVisibleMaterial();
}

bool ScriptPatchNode::subdivisionsFixed() const
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return false;

	return patchNode->getPatch().subdivisionsFixed();
}

Subdivisions ScriptPatchNode::getSubdivisions() const
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return Subdivisions();

	return patchNode->getPatch().getSubdivisions();
}

void ScriptPatchNode::setFixedSubdivisions(int isFixed, const Subdivisions& divisions)
{
	IPatchNodePtr patchNode = std::dynamic_pointer_cast<IPatchNode>(_node.lock());
	if (patchNode == NULL) return;

	patchNode->getPatch().setFixedSubdivisions(static_cast<bool>(isFixed), divisions);
}

// Initialise static members
const std::string ScriptPatchNode::_emptyShader;
PatchControl ScriptPatchNode::_emptyPatchControl;

ScriptSceneNode PatchInterface::createPatchDef2()
{
	// Create a new patch and return the script scene node
	scene::INodePtr node = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);

	// Add the node to the buffer otherwise it will be deleted immediately,
	// as ScriptSceneNodes are using weak_ptrs.
	SceneNodeBuffer::Instance().push_back(node);

	return ScriptSceneNode(node);
}

ScriptSceneNode PatchInterface::createPatchDef3()
{
	// Create a new patch and return the script scene node
	scene::INodePtr node = GlobalPatchModule().createPatch(patch::PatchDefType::Def3);

	// Add the node to the buffer otherwise it will be deleted immediately,
	// as ScriptSceneNodes are using weak_ptrs.
	SceneNodeBuffer::Instance().push_back(node);

	return ScriptSceneNode(node);
}

void PatchInterface::registerInterface(py::module& scope, py::dict& globals) 
{
	py::class_<PatchControl> patchControl(scope, "PatchMeshControl");
	patchControl.def_readwrite("vertex", &PatchControl::vertex);
	patchControl.def_readwrite("texcoord", &PatchControl::texcoord);

	py::class_<Subdivisions> subdivisions(scope, "Subdivisions");
	subdivisions.def(py::init<unsigned int, unsigned int>());
	subdivisions.def(py::init<const Subdivisions&>());

	// greebo: Pick the correct overload - this is hard to read, but it is necessary
	subdivisions.def("x", static_cast<unsigned int& (Subdivisions::*)()>(&Subdivisions::x), 
		py::return_value_policy::reference);
	subdivisions.def("y", static_cast<unsigned int& (Subdivisions::*)()>(&Subdivisions::y), 
		py::return_value_policy::reference);

	py::class_<VertexNT> patchVertex(scope, "PatchMeshVertex");

	patchVertex.def(py::init<>());
	patchVertex.def_readwrite("vertex", &VertexNT::vertex);
	patchVertex.def_readwrite("texcoord", &VertexNT::texcoord);
	patchVertex.def_readwrite("normal", &VertexNT::normal);

	// Declare the VertexNT vector
	py::bind_vector< std::vector<VertexNT> >(scope, "PatchMeshVertices");

	py::class_<PatchMesh> patchMesh(scope, "PatchMesh");
	patchMesh.def(py::init<>());
	patchMesh.def_readonly("width", &PatchMesh::width);
	patchMesh.def_readonly("height", &PatchMesh::height);
	patchMesh.def_readonly("vertices", &PatchMesh::vertices);

	// Define a PatchNode interface
	py::class_<ScriptPatchNode, ScriptSceneNode> patchNode(scope, "PatchNode");

	patchNode.def(py::init<const scene::INodePtr&>());
	patchNode.def("setDims", &ScriptPatchNode::setDims);
	patchNode.def("getWidth", &ScriptPatchNode::getWidth);
	patchNode.def("getHeight", &ScriptPatchNode::getHeight);
	patchNode.def("ctrlAt", &ScriptPatchNode::ctrlAt, py::return_value_policy::reference_internal);
	patchNode.def("insertColumns", &ScriptPatchNode::insertColumns);
	patchNode.def("insertRows", &ScriptPatchNode::insertRows);
	patchNode.def("removePoints", &ScriptPatchNode::removePoints);
	patchNode.def("appendPoints", &ScriptPatchNode::appendPoints);
	patchNode.def("isValid", &ScriptPatchNode::isValid);
	patchNode.def("isDegenerate", &ScriptPatchNode::isDegenerate);
	patchNode.def("getShader", &ScriptPatchNode::getShader, py::return_value_policy::reference);
	patchNode.def("setShader", &ScriptPatchNode::setShader);
	patchNode.def("hasVisibleMaterial", &ScriptPatchNode::hasVisibleMaterial);
	patchNode.def("subdivionsFixed", &ScriptPatchNode::subdivisionsFixed); // typo used to be there in previous releases, leave it in there for compatibility reasons
	patchNode.def("subdivisionsFixed", &ScriptPatchNode::subdivisionsFixed);
	patchNode.def("getSubdivisions", &ScriptPatchNode::getSubdivisions);
	patchNode.def("setFixedSubdivisions", &ScriptPatchNode::setFixedSubdivisions);
	patchNode.def("controlPointsChanged", &ScriptPatchNode::controlPointsChanged);
	patchNode.def("getTesselatedPatchMesh", &ScriptPatchNode::getTesselatedPatchMesh);

	// Define the GlobalPatchCreator interface
	py::class_<PatchInterface> patchCreator(scope, "PatchCreator");

	patchCreator.def("createPatchDef2", &PatchInterface::createPatchDef2);
	patchCreator.def("createPatchDef3", &PatchInterface::createPatchDef3);

	// Now point the Python variable "GlobalPatchCreator" to this instance
	globals["GlobalPatchCreator"] = this;
}

} // namespace script
