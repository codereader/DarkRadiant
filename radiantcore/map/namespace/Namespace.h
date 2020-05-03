#pragma once

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

	// INamespace implementation
	virtual void connect(const scene::INodePtr& root) override;
	virtual void disconnect(const scene::INodePtr& root) override;
	virtual bool nameExists(const std::string& name) override;
	virtual bool insert(const std::string& name) override;
	virtual bool erase(const std::string& name) override;
	virtual std::string addUniqueName(const std::string& originalName) override;
	virtual void addNameObserver(const std::string& name, NameObserver& observer) override;
	virtual void removeNameObserver(const std::string& name, NameObserver& observer) override;
	virtual void nameChanged(const std::string& oldName, const std::string& newName) override;
	virtual void ensureNoConflicts(const scene::INodePtr& root) override;
};
