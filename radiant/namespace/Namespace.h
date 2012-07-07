#pragma once

#include <map>
#include <vector>
#include "inamespace.h"
#include "iscenegraph.h"
#include "UniqueNameSet.h"

/// Implementing class for the INamespace module
class Namespace : public INamespace
{
	// The set of unique names in this namespace
	UniqueNameSet _uniqueNames;

	// The mapping between full names and observers, multiple keys allowed
	typedef std::multimap<std::string, NameObserver*> ObserverMap;
	ObserverMap _observers;

public:
	virtual ~Namespace();

	// INamespace implementation
	virtual void connect(const scene::INodePtr& root);
	virtual void disconnect(const scene::INodePtr& root);
	virtual bool nameExists(const std::string& name);
	virtual bool insert(const std::string& name);
	virtual bool erase(const std::string& name);
	virtual std::string addUniqueName(const std::string& originalName);
	virtual void addNameObserver(const std::string& name, NameObserver& observer);
	virtual void removeNameObserver(const std::string& name, NameObserver& observer);
	virtual void nameChanged(const std::string& oldName, const std::string& newName);
	virtual void ensureNoConflicts(const scene::INodePtr& root);
};
typedef boost::shared_ptr<Namespace> NamespacePtr;
