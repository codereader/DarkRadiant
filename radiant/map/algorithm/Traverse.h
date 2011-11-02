#pragma once

#include "inode.h"

namespace map {

/** greebo: Traverses the entire scenegraph (used as entry point/TraverseFunc for map saving)
 */
void traverse(const scene::INodePtr& root, scene::NodeVisitor& nodeExporter);

/** greebo: Traverses only the selected items
 */
void traverseSelected(const scene::INodePtr& root, scene::NodeVisitor& nodeExporter);

} // namespace map
