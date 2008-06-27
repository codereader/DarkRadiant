#ifndef BASICCONTAINER_H_
#define BASICCONTAINER_H_

#include "scenelib.h"
#include "inamespace.h"

namespace map {

/** 
 * greebo: This is a temporary container (node) used during map object import.
 * It possesses its own Namespace which all inserted child nodes get connected to.
 */
class BasicContainer : 
	public scene::Node
{};
typedef boost::shared_ptr<BasicContainer> BasicContainerPtr;

} // namespace map

#endif /*BASICCONTAINER_H_*/
