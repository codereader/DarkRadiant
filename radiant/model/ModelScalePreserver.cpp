#include "ModelScalePreserver.h"

#include "imapresource.h"
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
	forEachScaledModel(root, [](Entity& entity, model::ModelNode& model)
	{
		// Persist the modified scale by placing a special editor spawnarg
		entity.setKeyValue(MODELSCALE_KEY, string::to_string(model.getModelScale()));
	});
}

void ModelScalePreserver::onResourceExported(const scene::IMapRootNodePtr& root)
{
	// In this post-export event, we remove any scale spawnargs added earlier
	forEachScaledModel(root, [](Entity& entity, model::ModelNode& model)
	{
		if (!entity.getKeyValue(MODELSCALE_KEY).empty())
		{
			entity.setKeyValue(MODELSCALE_KEY, "");
		}
	});
}

}
