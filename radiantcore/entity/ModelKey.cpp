#include "ModelKey.h"

#include <functional>
#include "imodelcache.h"
#include "ifilter.h"
#include "modelskin.h"
#include "string/replace.h"
#include "scenelib.h"

ModelKey::ModelKey(scene::INode& parentNode) :
	_parentNode(parentNode),
	_active(true),
	_undo(_model, std::bind(&ModelKey::importState, this, std::placeholders::_1))
{}

const scene::INodePtr& ModelKey::getNode() const
{
	return _model.node;
}

void ModelKey::destroy()
{
    detachModelNode();

    _model.node.reset();
    _model.path.clear();
    _active = false;
}

void ModelKey::refreshModel()
{
	if (!_model.node) return;

    attachModelNodeKeepinSkin();
}

// Update the contained model from the provided keyvalues
void ModelKey::modelChanged(const std::string& value)
{
	if (!_active) return; // deactivated during parent node destruction

	// Sanitise the keyvalue - must use forward slashes
	auto newModelName = string::replace_all_copy(value, "\\", "/");

	if (newModelName == _model.path)
	{
		return; // new name is the same as we have now
	}

	_undo.save();

	// Now store the new modelpath
    _model.path = newModelName;

	// Call the attach routine, keeping the skin (#4142)
	attachModelNodeKeepinSkin();
}

void ModelKey::attachModelNode()
{
	// Remove the old model node first (this also clears the pointer)
    detachModelNode();

	// If the "model" spawnarg is empty, there's nothing to attach
    if (_model.path.empty()) return;

	// We have a non-empty model key, send the request to
	// the model cache to acquire a new child node
	_model.node = GlobalModelCache().getModelNode(_model.path);

	// The model loader should not return NULL, but a sanity check is always ok
	if (_model.node)
	{
		// Add the model node as child of the entity node
		_parentNode.addChildNode(_model.node);

		// Assign the model node to the same layers as the parent entity
		_model.node->assignToLayers(_parentNode.getLayers());

		// Inherit the parent node's visibility. This should do the trick to resolve #2709
		// but is not as heavy on performance as letting the Filtersystem check the whole subgraph

		// The sophisticated check would be like this
		// GlobalFilterSystem().updateSubgraph(_parentNode.getSelf());

		// Copy the visibility flags from the parent node (#4141 and #5134)
		scene::assignVisibilityFlagsFromNode(*_model.node, _parentNode);
	}
}

void ModelKey::detachModelNode()
{
    if (!_model.node) return; // nothing to do

    _parentNode.removeChildNode(_model.node);
    _model.node.reset();
}

void ModelKey::attachModelNodeKeepinSkin()
{
    if (_model.node)
    {
        // Check if we have a skinnable model and remember the skin
	    SkinnedModelPtr skinned = std::dynamic_pointer_cast<SkinnedModel>(_model.node);

	    std::string skin = skinned ? skinned->getSkin() : "";
	
	    attachModelNode();
	
	    // Reset the skin to the previous value if we have a model
	    skinned = std::dynamic_pointer_cast<SkinnedModel>(_model.node);

	    if (skinned)
	    {
		    skinned->skinChanged(skin);
	    }
    }
    else
    {
        // No existing model, just attach it
        attachModelNode();
    }
}

void ModelKey::skinChanged(const std::string& value)
{
	// Check if we have a skinnable model
	SkinnedModelPtr skinned = std::dynamic_pointer_cast<SkinnedModel>(_model.node);

	if (skinned)
	{
		skinned->skinChanged(value);
	}
}

void ModelKey::connectUndoSystem(IUndoSystem& undoSystem)
{
	_undo.connectUndoSystem(undoSystem);
}

void ModelKey::disconnectUndoSystem(IUndoSystem& undoSystem)
{
	_undo.disconnectUndoSystem(undoSystem);
}

void ModelKey::importState(const ModelNodeAndPath& data)
{
	_model.path = data.path;
	_model.node = data.node;
}
