#include "ModelKey.h"

#include <functional>

#include "entitylib.h"
#include "imodelcache.h"
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

    // The actual model path to request the model from the file cache
    std::string actualModelPath(_model.path);

    // Check if the model key is pointing to a def
    auto modelDef = GlobalEntityClassManager().findModel(_model.path);

    if (modelDef)
    {
        // We have a valid modelDef, use the mesh defined there
        actualModelPath = modelDef->getMesh();

        // Start watching the modelDef for changes
        subscribeToModelDef(modelDef);
    }

	// We have a non-empty model key, send the request to
	// the model cache to acquire a new child node
	_model.node = GlobalModelCache().getModelNode(actualModelPath);

	// The model loader should not return NULL, but a sanity check is always ok
    if (!_model.node) return;

	// Add the model node as child of the entity node
	_parentNode.addChildNode(_model.node);

	// Assign the model node to the same layers as the parent entity
	_model.node->assignToLayers(_parentNode.getLayers());

	// Inherit the parent node's visibility. This should do the trick to resolve #2709
	// but is not as heavy on performance as letting the Filtersystem check the whole subgraph

	// Copy the visibility flags from the parent node (#4141 and #5134)
	scene::assignVisibilityFlagsFromNode(*_model.node, _parentNode);

    // Assign idle pose to modelDef meshes
    if (modelDef)
    {
        scene::applyIdlePose(_model.node, modelDef);
    }

    // Mark the transform of this model as changed, it must re-evaluate itself
    _model.node->transformChanged();
}

void ModelKey::detachModelNode()
{
    unsubscribeFromModelDef();

    if (!_model.node) return; // nothing to do

    _parentNode.removeChildNode(_model.node);
    _model.node.reset();
}

void ModelKey::onModelDefChanged()
{
    attachModelNodeKeepinSkin();
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
	auto skinned = std::dynamic_pointer_cast<SkinnedModel>(_model.node);

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
    _model.modelDefMonitored = data.modelDefMonitored;

    if (_model.modelDefMonitored)
    {
        unsubscribeFromModelDef();

        if (auto modelDef = GlobalEntityClassManager().findModel(_model.path); modelDef)
        {
            subscribeToModelDef(modelDef);
        }
    }
}

void ModelKey::subscribeToModelDef(const IModelDef::Ptr& modelDef)
{
    // Monitor this modelDef for potential mesh changes
    _modelDefChanged = modelDef->signal_DeclarationChanged().connect(
        sigc::mem_fun(this, &ModelKey::onModelDefChanged)
    );
    _model.modelDefMonitored = true;
}

void ModelKey::unsubscribeFromModelDef()
{
    _modelDefChanged.disconnect();
    _model.modelDefMonitored = false;
}
