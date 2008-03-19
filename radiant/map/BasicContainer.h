#ifndef BASICCONTAINER_H_
#define BASICCONTAINER_H_

#include "scenelib.h"

namespace map {

/** greebo: This is a temporary container (node) used during map object import.
 * 			It implements a Traversable, which allows to add child nodes.
 */
class BasicContainer : 
	public scene::Node
{
public:
	BasicContainer()
	{}
};

} // namespace map

#endif /*BASICCONTAINER_H_*/
