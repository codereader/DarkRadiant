#include "Skins.h"

#include "modelskin.h"
#include "scenelib.h"
#include "ui/modelselector/ModelSelector.h"

namespace map
{

namespace algorithm
{

class RefreshSkinWalker :
    public scene::NodeVisitor
{
public:
    bool pre(const scene::INodePtr& node)
	{
        // Check if we have a skinnable model
        SkinnedModelPtr skinned = boost::dynamic_pointer_cast<SkinnedModel>(node);

        if (skinned != NULL)
		{
            // Let the skinned model reload its current skin.
            skinned->skinChanged(skinned->getSkin());
        }

        return true; // traverse children
    }
};

void reloadSkins(const cmd::ArgumentList& args)
{
    GlobalModelSkinCache().refresh();

    RefreshSkinWalker walker;
    Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

    // Refresh the ModelSelector too
    ui::ModelSelector::refresh();
}

} // namespace

} // namespace
