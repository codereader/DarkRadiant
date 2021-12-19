#include "ModelScalePreserver.h"

#include "ientity.h"
#include "itransformable.h"
#include "imapresource.h"
#include "itextstream.h"
#include "string/convert.h"

namespace map
{

namespace
{
	const char* const MODELSCALE_KEY = "editor_modelScale";
}

ModelScalePreserver::ModelScalePreserver() :
	_modelScaleKey(MODELSCALE_KEY)
{
	// #5220: To cover having the scale of resized models preserved in
	// auto-saves and prefabs, we subscribe to the exporting events
	// and check for any models that still have a modified scale on it.
	// That scale value is then written to the hosting entity's spawnargs.
	GlobalMapResourceManager().signal_onResourceExporting().connect(
		sigc::mem_fun(this, &ModelScalePreserver::onResourceExporting)
	);
	GlobalMapResourceManager().signal_onResourceExported().connect(
		sigc::mem_fun(this, &ModelScalePreserver::onResourceExported)
	);

	// After map loading this class will try to reconstruct the scale
	GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(this, &ModelScalePreserver::onMapEvent)
	);
}

void ModelScalePreserver::forEachScaledModel(const scene::IMapRootNodePtr& root,
	const std::function<void(Entity&, model::ModelNode&)>& func)
{
	root->foreachNode([&](const scene::INodePtr& node)
	{
		if (Node_isEntity(node))
		{
			// Find any model nodes below that one
			node->foreachNode([&](const scene::INodePtr& child)
			{
				model::ModelNodePtr model = Node_getModel(child);

				if (model && model->hasModifiedScale())
				{
					// Found a model with modified scale
					func(*Node_getEntity(node), *model);
				}

				return true;
			});
		}

		return true;
	});
}

void ModelScalePreserver::onResourceExporting(const scene::IMapRootNodePtr& root)
{
	// Traverse the exported scene and check for any models that are still scaled, to
	// persist that value in the exported scene.
	// In "regular" map saves, all models already have been processed here at this point,
	// and their scale is reset, so in this case the following traversal does nothing.
	forEachScaledModel(root, [this](Entity& entity, model::ModelNode& model)
	{
		// Persist the modified scale by placing a special editor spawnarg
		entity.setKeyValue(_modelScaleKey, string::to_string(model.getModelScale()));
	});
}

void ModelScalePreserver::onResourceExported(const scene::IMapRootNodePtr& root)
{
	// In this post-export event, we remove any scale spawnargs added earlier
	forEachScaledModel(root, [this](Entity& entity, model::ModelNode& model)
	{
		if (!entity.getKeyValue(_modelScaleKey).empty())
		{
			entity.setKeyValue(_modelScaleKey, "");
		}
	});
}

void ModelScalePreserver::restoreModelScale(const scene::IMapRootNodePtr& root)
{
	root->foreachNode([this](const scene::INodePtr& node)
	{
		if (Node_isEntity(node))
		{
			Entity* entity = Node_getEntity(node);

			// Search for the editor_ key and apply the scale if found
			auto savedScale = entity->getKeyValue(_modelScaleKey);

			if (!savedScale.empty())
			{
				Vector3 scale = string::convert<Vector3>(savedScale);

				// Find any model nodes below that one
				node->foreachNode([&](const scene::INodePtr& child)
				{
					model::ModelNodePtr model = Node_getModel(child);
					ITransformablePtr transformable = scene::node_cast<ITransformable>(child);

					if (model && transformable)
					{
						rMessage() << "Restoring model scale on node " << child->name() << std::endl;

						transformable->setType(TRANSFORM_PRIMITIVE);
						transformable->setScale(scale);
						transformable->freezeTransform();
					}

					return true;
				});

				// Clear the spawnarg now that we've applied it
				entity->setKeyValue(_modelScaleKey, "");
			}
		}

		return true;
	});
}

void ModelScalePreserver::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapLoaded)
	{
		// After loading, restore the scale if it gets recovered
		restoreModelScale(GlobalMapModule().getRoot());
	}
}

}
