#include "ModelInterface.h"

#include <pybind11/pybind11.h>
#include "imodelsurface.h"
#include "modelskin.h"

namespace script
{

int ScriptModelSurface::getNumVertices() const
{
	return _surface.getNumVertices();
}

int ScriptModelSurface::getNumTriangles() const
{
	return _surface.getNumTriangles();
}

const MeshVertex& ScriptModelSurface::getVertex(int vertexIndex) const
{
	return _surface.getVertex(vertexIndex);
}

model::ModelPolygon ScriptModelSurface::getPolygon(int polygonIndex) const
{
	return _surface.getPolygon(polygonIndex);
}

std::string ScriptModelSurface::getDefaultMaterial() const
{
	return _surface.getDefaultMaterial();
}

std::string ScriptModelSurface::getActiveMaterial() const
{
	return _surface.getActiveMaterial();
}

// ----------- ScriptModelNode -----------

// Constructor, checks if the passed node is actually an entity
ScriptModelNode::ScriptModelNode(const scene::INodePtr& node) :
	ScriptSceneNode((node != NULL && Node_isModel(node)) ? node : scene::INodePtr())
{}

std::string ScriptModelNode::getFilename()
{
	model::ModelNodePtr modelNode = Node_getModel(*this);
	if (modelNode == NULL) return "";

	return modelNode->getIModel().getFilename();
}

std::string ScriptModelNode::getModelPath()
{
	model::ModelNodePtr modelNode = Node_getModel(*this);
	if (modelNode == NULL) return "";

	return modelNode->getIModel().getModelPath();
}

int ScriptModelNode::getSurfaceCount()
{
	model::ModelNodePtr modelNode = Node_getModel(*this);
	if (modelNode == NULL) return -1;

	return modelNode->getIModel().getSurfaceCount();
}

int ScriptModelNode::getVertexCount()
{
	model::ModelNodePtr modelNode = Node_getModel(*this);
	if (modelNode == NULL) return -1;

	return modelNode->getIModel().getVertexCount();
}

int ScriptModelNode::getPolyCount()
{
	model::ModelNodePtr modelNode = Node_getModel(*this);
	if (modelNode == NULL) return -1;

	return modelNode->getIModel().getPolyCount();
}

ScriptModelSurface ScriptModelNode::getSurface(int surfaceNum)
{
	model::ModelNodePtr modelNode = Node_getModel(*this);
	if (modelNode == NULL) throw std::runtime_error("Empty model node.");

	return ScriptModelSurface(modelNode->getIModel().getSurface(surfaceNum));
}

model::StringList ScriptModelNode::getActiveMaterials()
{
	model::ModelNodePtr modelNode = Node_getModel(*this);
	if (modelNode == NULL) return model::StringList();

	// Get the list of default shaders from this model, this is without any skins applied
	model::StringList materials = modelNode->getIModel().getActiveMaterials();

	// Check if the model is a skinned one, so let's check for active skins
	SkinnedModelPtr skinnedModel = std::dynamic_pointer_cast<SkinnedModel>(modelNode);

	if (skinnedModel != NULL)
	{
		// This is a skinned model, get the surface remap
		std::string curSkin = skinnedModel->getSkin();

		auto skin = GlobalModelSkinCache().findSkin(curSkin);

        if (skin)
        {
            for (auto& material : materials)
            {
                std::string remap = skin->getRemap(material);

                if (remap.empty()) continue;

                // Remapping found, use this material instead of the default material
                material = remap;
            }
        }
	}

	return materials;
}

// Checks if the given SceneNode structure is a ModelNode
bool ScriptModelNode::isModel(const ScriptSceneNode& node) {
	return Node_isModel(node);
}

// "Cast" service for Python, returns a ScriptModelNode.
// The returned node is non-NULL if the cast succeeded
ScriptModelNode ScriptModelNode::getModel(const ScriptSceneNode& node) {
	// Try to cast the node onto a model
	model::ModelNodePtr modelNode = Node_getModel(node);

	// Construct a modelNode (contained node is NULL if not a model)
	return ScriptModelNode(modelNode != NULL
                           ? node
                           : ScriptSceneNode(scene::INodePtr()));
}

void ModelInterface::registerInterface(py::module& scope, py::dict& globals)
{
	py::class_<MeshVertex> vertex(scope, "MeshVertex");

	vertex.def_readwrite("texcoord", &MeshVertex::texcoord);
	vertex.def_readwrite("normal", &MeshVertex::normal);
	vertex.def_readwrite("vertex", &MeshVertex::vertex);
	vertex.def_readwrite("tangent", &MeshVertex::tangent);
	vertex.def_readwrite("bitangent", &MeshVertex::bitangent);
	vertex.def_readwrite("colour", &MeshVertex::colour);

    // Register the old name as alias to MeshVertex
    scope.add_object("ArbitraryMeshVertex", vertex);

	py::class_<model::ModelPolygon> poly(scope, "ModelPolygon");

	poly.def_readonly("a", &model::ModelPolygon::a);
	poly.def_readonly("b", &model::ModelPolygon::b);
	poly.def_readonly("c", &model::ModelPolygon::c);

	// Add the ModelSurface interface
	py::class_<ScriptModelSurface> surface(scope, "ModelSurface");

	surface.def(py::init<const model::IModelSurface&>());
	surface.def("getNumVertices", &ScriptModelSurface::getNumVertices);
	surface.def("getNumTriangles", &ScriptModelSurface::getNumTriangles);
	surface.def("getVertex", &ScriptModelSurface::getVertex, py::return_value_policy::reference);
	surface.def("getPolygon", &ScriptModelSurface::getPolygon);
	surface.def("getDefaultMaterial", &ScriptModelSurface::getDefaultMaterial);
	surface.def("getActiveMaterial", &ScriptModelSurface::getActiveMaterial);

	// Add the ModelNode interface
	py::class_<ScriptModelNode, ScriptSceneNode> modelNode(scope, "ModelNode");

	modelNode.def(py::init<const scene::INodePtr&>());
	modelNode.def("getFilename", &ScriptModelNode::getFilename);
	modelNode.def("getModelPath", &ScriptModelNode::getModelPath);
	modelNode.def("getSurfaceCount", &ScriptModelNode::getSurfaceCount);
	modelNode.def("getVertexCount", &ScriptModelNode::getVertexCount);
	modelNode.def("getPolyCount", &ScriptModelNode::getPolyCount);
	modelNode.def("getActiveMaterials", &ScriptModelNode::getActiveMaterials);
	modelNode.def("getSurface", &ScriptModelNode::getSurface);
}

} // namespace script
