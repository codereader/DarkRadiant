#include "ModelInterface.h"

#include "modelskin.h"
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace script {

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

//void applySkin(const ModelSkin& skin);

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

model::MaterialList ScriptModelNode::getActiveMaterials()
{
	model::ModelNodePtr modelNode = Node_getModel(*this);
	if (modelNode == NULL) return model::MaterialList();

	// Get the list of default shaders from this model, this is without any skins applied
	model::MaterialList materials = modelNode->getIModel().getActiveMaterials();

	// Check if the model is a skinned one, so let's check for active skins
	SkinnedModelPtr skinnedModel = boost::dynamic_pointer_cast<SkinnedModel>(modelNode);

	if (skinnedModel != NULL)
	{
		// This is a skinned model, get the surface remap
		std::string curSkin = skinnedModel->getSkin();

		ModelSkin& skinInfo = GlobalModelSkinCache().capture(curSkin);

		for (model::MaterialList::iterator i = materials.begin(); i != materials.end(); ++i)
		{
			std::string remap = skinInfo.getRemap(*i);

			if (remap.empty()) continue;

			// Remapping found, use this material instead of the default material
			*i = remap;
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

void ModelInterface::registerInterface(boost::python::object& nspace)
{
	// Add the ModelNode interface
	nspace["ModelNode"] = boost::python::class_<ScriptModelNode, 
		boost::python::bases<ScriptSceneNode> >("ModelNode", boost::python::init<const scene::INodePtr&>() )
		.def("getFilename", &ScriptModelNode::getFilename)
		.def("getModelPath", &ScriptModelNode::getModelPath)
		.def("getSurfaceCount", &ScriptModelNode::getSurfaceCount)
		.def("getVertexCount", &ScriptModelNode::getVertexCount)
		.def("getPolyCount", &ScriptModelNode::getPolyCount)
		.def("getActiveMaterials", &ScriptModelNode::getActiveMaterials)
	;

	// Add the "isModel" and "getModel" methods to all ScriptSceneNodes
	boost::python::object sceneNode = nspace["SceneNode"];

	boost::python::objects::add_to_namespace(sceneNode, 
		"isModel", boost::python::make_function(&ScriptModelNode::isModel));

	boost::python::objects::add_to_namespace(sceneNode, 
		"getModel", boost::python::make_function(&ScriptModelNode::getModel));
}

} // namespace script
