#pragma once

#include "../PatchConstants.h"

namespace patch
{

namespace algorithm
{

/**
 * Construct a patch prefab of the given type, size and AABB.
 * This method will de-select all items in the scene, and the newly created
 * item will be selected.
 */
void constructPrefab(const AABB& aabb, const std::string& shader, EPatchPrefab eType, 
					 EViewType viewType, std::size_t width = 3, std::size_t height = 3);

} // namespace

} // namespace
