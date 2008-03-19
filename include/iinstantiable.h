#ifndef IINSTANTIABLE_H_
#define IINSTANTIABLE_H_

#include <boost/shared_ptr.hpp>

namespace scene {

class Path;

class Instantiable
{
public:
	/**
	 * greebo: Notifies this instantiable about the instantiation. The given 
	 *         path refers to the position in the scenegraph this item is 
	 *         inserted into.
	 */
	virtual void instantiate(const Path& path) = 0;

	/**
	 * greebo: The counterpart of the above method. Notifies this item 
	 *         that it has been removed from the given scene::Path.
	 */
	virtual void uninstantiate(const Path& path) = 0;
};
typedef boost::shared_ptr<Instantiable> InstantiablePtr;

} // namespace scene

#endif /*IINSTANTIABLE_H_*/
