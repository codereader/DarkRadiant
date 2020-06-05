#include "MapCommands.h"

#include "prefabselector/PrefabSelector.h"

namespace ui
{

void loadPrefabDialog(const cmd::ArgumentList& args)
{
    auto result = PrefabSelector::ChoosePrefab();

    if (!result.prefabPath.empty())
    {
        GlobalCommandSystem().executeCommand(LOAD_PREFAB_AT_CMD, Vector3(0, 0, 0));
    }
}

}
