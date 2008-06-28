#ifndef NAMESPACE_H_
#define NAMESPACE_H_

#include <map>
#include <vector>
#include "inamespace.h"
#include "iscenegraph.h"
#include "UniqueNameSet.h"

class Namespace : 
	public INamespace
{
	// The set of unique names in this namespace
	UniqueNameSet _uniqueNames;
	
	// The mapping between full names and observers, multiple keys allowed
	typedef std::multimap<std::string, NameObserver*> ObserverMap;
	ObserverMap _observers;

public:
	virtual ~Namespace();

	// Documentation: see inamespace.h
	virtual void connect(const scene::INodePtr& root);
	virtual void disconnect(const scene::INodePtr& root);

	// Returns TRUE if the name already exists in this namespace
	virtual bool nameExists(const std::string& name);

	// Inserts a new name into the namespace, returns TRUE on success
	virtual bool insert(const std::string& name);
	virtual bool erase(const std::string& name);

	// Returns a new, unique string which is not yet used in this namespace
	// For the string is automatically registered in this namespace
	virtual std::string makeUniqueAndInsert(const std::string& originalName);

	// Returns a new, unique string which is not yet used in this namespace
	virtual std::string makeUnique(const std::string& originalName);

	// Add or remove a nameobserver
	virtual void addNameObserver(const std::string& name, NameObserver& observer);
	virtual void removeNameObserver(const std::string& name, NameObserver& observer);

	// Broadcasts the nameChanged event
	virtual void nameChanged(const std::string& oldName, const std::string& newName);

	// Imports all names below root into this namespace, changing names where needed
	virtual void importNames(const scene::INodePtr& root);
};
typedef boost::shared_ptr<Namespace> NamespacePtr;

#endif /*NAMESPACE_H_*/
