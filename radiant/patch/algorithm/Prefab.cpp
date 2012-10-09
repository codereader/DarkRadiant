#include "Prefab.h"

#include "iselection.h"
#include "ipatch.h"

#include "../Patch.h"
#include "../../map/Map.h"

namespace patch
{

namespace algorithm
{

void constructPrefab(const AABB& aabb, const std::string& shader, EPatchPrefab eType, 
					 EViewType viewType, std::size_t width, std::size_t height)
{
	GlobalSelectionSystem().setSelectedAll(false);

	scene::INodePtr node(GlobalPatchCreator(DEF2).createPatch());
	GlobalMap().findOrInsertWorldspawn()->addChildNode(node);

	Patch* patch = Node_getPatch(node);
	patch->setShader(shader);

	patch->ConstructPrefab(aabb, eType, viewType, width, height);
	patch->controlPointsChanged();

	Node_setSelected(node, true);
}

}

} // namespace
