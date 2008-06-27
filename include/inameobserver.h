#ifndef _NAME_OBSERVER_H_
#define _NAME_OBSERVER_H_

#include <string>
#include <boost/shared_ptr.hpp>

/**
 * greebo: A NameObserver is observing one name in a map Namespace.
 *
 * It provides "event methods" which get called by the Namespace on
 * any name changes.
 */
class NameObserver
{
public:
	/** 
	 * greebo: This is the "change" event, which gets issued by the Namespace.
	 * The old name as well as the new name is passed along.
	 */
	virtual void onNameChange(const std::string& oldName, const std::string& newName) = 0;
};
typedef boost::shared_ptr<NameObserver> NameObserverPtr;

#endif /* _NAME_OBSERVER_H_ */
