#ifndef TRAVERSE_
#define TRAVERSE_

#include "inode.h"

namespace map {

/** greebo: Traverses the entire scenegraph (used as entry point/TraverseFunc for map saving)
 */
void traverse(scene::INodePtr root, scene::NodeVisitor& nodeExporter);

/** greebo: Traverses only the selected items
 */
void traverseSelected(scene::INodePtr root, scene::NodeVisitor& nodeExporter);

} // namespace map

#endif /*TRAVERSE_*/
