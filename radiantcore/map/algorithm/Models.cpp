#include "Models.h"

#include <set>

#include "i18n.h"
#include "inode.h"
#include "iselection.h"
#include "ientity.h"
#include "imodel.h"
#include "imodelcache.h"
#include "iscenegraph.h"

#include "messages/ScopedLongRunningOperation.h"

namespace map
{

namespace algorithm
{

class ModelFinder :
    public selection::SelectionSystem::Visitor,
	public scene::NodeVisitor
{
public:
	typedef std::set<IEntityNodePtr> Entities;
	typedef std::set<std::string> ModelPaths;

private:
	// All the model path of the selected modelnodes
	mutable ModelPaths _modelNames;
	// All the selected entities with modelnodes as child
	mutable Entities _entities;

public:
	bool pre(const scene::INodePtr& node)
	{
		model::ModelNodePtr model = Node_getModel(node);

		if (model)
		{
			_modelNames.insert(model->getIModel().getModelPath());

			IEntityNodePtr ent = std::dynamic_pointer_cast<IEntityNode>(node->getParent());

			if (ent)
			{
				_entities.insert(ent);
			}

			return false;
		}

		return true;
	}

	void visit(const scene::INodePtr& node) const
	{
		node->traverse(*const_cast<ModelFinder*>(this));
	}

	const Entities& getEntities() const
	{
		return _entities;
	}

	const ModelPaths& getModelPaths() const
	{
		return _modelNames;
	}
};

class ModelRefreshWalker :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node)
	{
		IEntityNodePtr entity = std::dynamic_pointer_cast<IEntityNode>(node);

		if (entity)
		{
			entity->refreshModel();
			return false;
		}

		return true;
	}
};

void refreshModels(bool blockScreenUpdates)
{
	std::unique_ptr<radiant::ScopedLongRunningOperation> blocker;

	if (blockScreenUpdates)
	{
		// Disable screen updates for the scope of this function
		blocker.reset(new radiant::ScopedLongRunningOperation(_("Reloading Models")));
	}

	// Clear the model cache
	GlobalModelCache().clear();

	// Update all model nodes
	ModelRefreshWalker walker;
	GlobalSceneGraph().root()->traverse(walker);

	// Send the signal to the UI
	GlobalModelCache().signal_modelsReloaded().emit();
}

void refreshSelectedModels(bool blockScreenUpdates)
{
	std::unique_ptr<radiant::ScopedLongRunningOperation> blocker;

	if (blockScreenUpdates)
	{
		// Disable screen updates for the scope of this function
		blocker.reset(new radiant::ScopedLongRunningOperation(_("Reloading Models")));
	}

	// Find all models in the current selection
	ModelFinder walker;
	GlobalSelectionSystem().foreachSelected(walker);

	// Remove the selected models from the cache
	ModelFinder::ModelPaths models = walker.getModelPaths();

	for (const std::string& modelPath : models)
	{
		GlobalModelCache().removeModel(modelPath);
	}

	// Traverse the entities and submit a refresh call
	ModelFinder::Entities entities = walker.getEntities();

	for (const IEntityNodePtr& entityNode : entities)
	{
		entityNode->refreshModel();
	}
}

// Reloads all entities with their model spawnarg referencing the given model path.
// The given model path denotes a VFS path, i.e. it is mod/game-relative
void refreshModelsByPath(const std::string& relativeModelPath)
{
    std::size_t refreshedEntityCount = 0;

    GlobalModelCache().removeModel(relativeModelPath);

    GlobalMapModule().getRoot()->foreachNode([&](const scene::INodePtr& node)
    {
        auto entity = std::dynamic_pointer_cast<IEntityNode>(node);

        if (entity && entity->getEntity().getKeyValue("model") == relativeModelPath)
        {
            entity->refreshModel();
            ++refreshedEntityCount;
        }

        return true;
    });

    rMessage() << "Refreshed " << refreshedEntityCount << " entities using the model " << relativeModelPath << std::endl;
}

}

}
