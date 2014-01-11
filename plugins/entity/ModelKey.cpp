#include "ModelKey.h"

#include "imodelcache.h"
#include "ifiletypes.h"
#include "scene/Node.h"
#include "ifilter.h"
#include "modelskin.h"
#include <boost/algorithm/string/replace.hpp>

ModelKey::ModelKey(scene::INode& parentNode) :
	_parentNode(parentNode),
	_active(true)
{}

const scene::INodePtr& ModelKey::getNode() const
{
	return _modelNode;
}

void ModelKey::setActive(bool active)
{
	_active = active;
}

void ModelKey::refreshModel()
{
	if (!_modelNode) return;

	// Check if we have a skinnable model and remember the skin
	SkinnedModelPtr skinned = boost::dynamic_pointer_cast<SkinnedModel>(_modelNode);

	std::string skin = skinned ? skinned->getSkin() : "";
	
	attachModelNode();
	
	// Reset the skin to the previous value if we have a model
	skinned = boost::dynamic_pointer_cast<SkinnedModel>(_modelNode);

	if (skinned)
	{
		skinned->skinChanged(skin);
	}
}

// Update the contained model from the provided keyvalues
void ModelKey::modelChanged(const std::string& value)
{
	if (!_active) return; // deactivated during parent node destruction

	// Sanitise the keyvalue - must use forward slashes
	std::string newModelName = boost::algorithm::replace_all_copy(value, "\\", "/");

	if (newModelName == _modelPath)
	{
		return; // new name is the same as we have now
	}

	// Now store the new modelpath
    _modelPath = newModelName;

	// Call the attach routine
	attachModelNode();
}

void ModelKey::attachModelNode()
{
	// Remove the old model node first
	if (_modelNode != NULL)
	{
		_parentNode.removeChildNode(_modelNode);
	}

	if (_modelPath.empty())
	{
		// Empty "model" spawnarg, clear the pointer and exit
		_modelNode = scene::INodePtr();
		return;
	}

	// We have a non-empty model key, send the request to
	// the model cache to acquire a new child node
	_modelNode = GlobalModelCache().getModelNode(_modelPath);

	// The model loader should not return NULL, but a sanity check is always ok
	if (_modelNode)
	{
		// Add the model node as child of the entity node
		_parentNode.addChildNode(_modelNode);

		// Assign the model node to the same layers as the parent entity
		_modelNode->assignToLayers(_parentNode.getLayers());

		// Inherit the parent node's visibility. This should do the trick to resolve #2709
		// but is not as heavy on performance as letting the Filtersystem check the whole subgraph

		// The sophisticated check would be like this
		// GlobalFilterSystem().updateSubgraph(_parentNode.getSelf());
		
		_modelNode->setFiltered(_parentNode.isFiltered());

		if (_parentNode.excluded())
		{
			_modelNode->enable(scene::Node::eExcluded);
		}
	}
}

void ModelKey::skinChanged(const std::string& value)
{
	// Check if we have a skinnable model
	SkinnedModelPtr skinned = boost::dynamic_pointer_cast<SkinnedModel>(_modelNode);

	if (skinned)
	{
		skinned->skinChanged(value);
	}
}
