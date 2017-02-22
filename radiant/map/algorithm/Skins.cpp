#include "Skins.h"

#include "iscenegraph.h"
#include "modelskin.h"

namespace map
{

namespace algorithm
{

void reloadSkins(const cmd::ArgumentList& args)
{
	// This will emit a signal refreshing the ModelSelector too
    GlobalModelSkinCache().refresh();

	GlobalSceneGraph().foreachNode([] (const scene::INodePtr& node)->bool
	{
		// Check if we have a skinnable model
        SkinnedModelPtr skinned = std::dynamic_pointer_cast<SkinnedModel>(node);

        if (skinned)
		{
            // Let the skinned model reload its current skin.
            skinned->skinChanged(skinned->getSkin());
        }

        return true; // traverse further
	});
}

} // namespace

} // namespace
