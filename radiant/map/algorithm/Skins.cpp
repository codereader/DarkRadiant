#include "Skins.h"

#include "modelskin.h"
#include "ui/modelselector/ModelSelector.h"

namespace map
{

namespace algorithm
{

void reloadSkins(const cmd::ArgumentList& args)
{
    GlobalModelSkinCache().refresh();

	GlobalSceneGraph().foreachNode([] (const scene::INodePtr& node)->bool
	{
		// Check if we have a skinnable model
        SkinnedModelPtr skinned = boost::dynamic_pointer_cast<SkinnedModel>(node);

        if (skinned)
		{
            // Let the skinned model reload its current skin.
            skinned->skinChanged(skinned->getSkin());
        }

        return true; // traverse further
	});

    // Refresh the ModelSelector too
    ui::ModelSelector::refresh();
}

} // namespace

} // namespace
