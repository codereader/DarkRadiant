#pragma once

#include "inode.h"
#include "icommandsystem.h"
#include <utility>

namespace scene
{

const char* const SELECT_NODE_BY_INDEX_CMD = "SelectNodeByIndex";

/**
 * Returns the entity and brush number for the given node.
 * Throws std::out_of_range if the node could not be found in the current scene.
 *
 * The given node must be an entity or a primitive, otherwise a std::out_of_range 
 * exceptions will be thrown.
 */
std::pair<std::size_t, std::size_t> getNodeIndices(const scene::INodePtr& node);

void selectNodeByIndex(const cmd::ArgumentList& args);

}
