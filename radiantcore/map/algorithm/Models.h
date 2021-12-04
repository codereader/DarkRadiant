#pragma once

#include <string>

namespace map
{

namespace algorithm
{

// This reloads all models in the map
void refreshModels(bool blockScreenUpdates);

// This reloads all selected models in the map
void refreshSelectedModels(bool blockScreenUpdates);

// Reloads all entities with their model spawnarg referencing the given model path.
// The given model path denotes a VFS path, i.e. it is mod/game-relative
void refreshModelsByPath(const std::string& relativeModelPath);

}

}

