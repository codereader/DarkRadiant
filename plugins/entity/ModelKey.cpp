#include "ModelKey.h"

#include "imodelcache.h"
#include "ifiletypes.h"
#include "scenelib.h"
#include "ifilter.h"
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

// Update the contained model from the provided keyvalues
void ModelKey::modelChanged(const std::string& value)
{
	if (!_active) return; // deactivated during parent node destruction

	// Remove the old model node first
	if (_modelNode != NULL)
	{
		_parentNode.removeChildNode(_modelNode);
	}

	// Now store the new modelpath
    // Sanitise the keyvalue - must use forward slashes
	_modelPath = boost::algorithm::replace_all_copy(value, "\\", "/");

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
	if (_modelNode != NULL)
	{
		// Add the model node as child of the entity node
		_parentNode.addChildNode(_modelNode);

		// Assign the model node to the same layers as the parent entity
		scene::assignNodeToLayers(_modelNode, _parentNode.getLayers());

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
