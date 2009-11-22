#ifndef IINSTANTIABLE_H_
#define IINSTANTIABLE_H_

#include <boost/shared_ptr.hpp>

namespace scene {

class Instantiable
{
public:
    /**
	 * Destructor
	 */
	virtual ~Instantiable() {}

	/**
	 * greebo: Notifies this instantiable about the instantiation. 
	 */
	virtual void instantiate() = 0;

	/**
	 * greebo: The counterpart of the above method.
	 */
	virtual void uninstantiate() = 0;
};
typedef boost::shared_ptr<Instantiable> InstantiablePtr;

} // namespace scene

#endif /*IINSTANTIABLE_H_*/
