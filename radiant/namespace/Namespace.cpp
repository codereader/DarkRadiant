#include "Namespace.h"

#include "scenelib.h"
#include <list>
#include "itextstream.h"
#include "modulesystem/StaticModule.h"

class ConnectNamespacedWalker : 
	public scene::NodeVisitor
{
	Namespace* _nspace;
public:
	ConnectNamespacedWalker(Namespace* nspace) :
		_nspace(nspace)
	{}

	virtual bool pre(const scene::INodePtr& node) {
		NamespacedPtr namespaced = Node_getNamespaced(node);

		if (namespaced == NULL) {
			return true;
		}

		INamespace* foreignNamespace = namespaced->getNamespace();

		if (foreignNamespace != NULL && foreignNamespace != _nspace) {
			// The node is already connected to a different namespace, disconnect
			namespaced->disconnectNameObservers();
			namespaced->detachNames();
			namespaced->setNamespace(NULL);
		}

		// Set the namespace reference and add all "names"
		namespaced->setNamespace(_nspace);
		namespaced->attachNames();

		return true;
	}
};

class DisconnectNamespacedWalker :
	public scene::NodeVisitor
{
public:
	virtual bool pre(const scene::INodePtr& node) {
		NamespacedPtr namespaced = Node_getNamespaced(node);

		// Only do this, if the item is Namespaced and attached to a Namespace
		if (namespaced != NULL && namespaced->getNamespace() != NULL) {
			// Remove names and clear namespace
			namespaced->detachNames();
			namespaced->setNamespace(NULL);
		}

		return true;
	}
};

class ConnectNameObserverWalker :
	public scene::NodeVisitor
{
public:
	virtual bool pre(const scene::INodePtr& node) {
		NamespacedPtr namespaced = Node_getNamespaced(node);

		// Only do this, if the item is Namespaced and attached to a Namespace
		if (namespaced != NULL && namespaced->getNamespace() != NULL) {
			namespaced->connectNameObservers();
		}

		return true;
	}
};

class DisconnectNameObserverWalker :
	public scene::NodeVisitor
{
public:
	virtual bool pre(const scene::INodePtr& node) {
		NamespacedPtr namespaced = Node_getNamespaced(node);

		// Only do this, if the item is Namespaced and attached to a Namespace
		if (namespaced != NULL && namespaced->getNamespace() != NULL) {
			namespaced->disconnectNameObservers();
		}

		return true;
	}
};

// A walker importing all names into this namespace
class GatherNamespacedWalker : 
	public scene::NodeVisitor
{
	// The set of imported names to keep track of what got imported already
	std::set<NamespacedPtr>& _targetSet;

public:
	GatherNamespacedWalker(std::set<NamespacedPtr>& targetSet) :
		_targetSet(targetSet)
	{}

	virtual bool pre(const scene::INodePtr& node) {
		NamespacedPtr namespaced = Node_getNamespaced(node);

		if (namespaced != NULL) {
			// Insert this
			_targetSet.insert(namespaced);
		}

		return true;
	}
};

void Namespace::connect(const scene::INodePtr& root) {
	// Now traverse the subgraph and connect the nodes
	ConnectNamespacedWalker firstWalker(this);
	Node_traverseSubgraph(root, firstWalker);

	ConnectNameObserverWalker secondWalker;
	Node_traverseSubgraph(root, secondWalker);
}

void Namespace::disconnect(const scene::INodePtr& root) {
	// First, disconnect all NameObservers
	DisconnectNameObserverWalker firstWalker;
	Node_traverseSubgraph(root, firstWalker);

	// Second, remove all "names" from the namespace and clear the reference
	DisconnectNamespacedWalker secondWalker;
	Node_traverseSubgraph(root, secondWalker);
}

bool Namespace::nameExists(const std::string& name) {
	return _uniqueNames.nameExists(name);
}

bool Namespace::insert(const std::string& name) {
	return _uniqueNames.insert(name);
}

bool Namespace::erase(const std::string& name) {
	return _uniqueNames.erase(name);
}

std::string Namespace::makeUniqueAndInsert(const std::string& originalName) {
	// Create the maintainable object out of the given full name
	ComplexName name(originalName);

	// Make this name unique and update the set
	_uniqueNames.makeUniqueAndInsert(name);

	// Return the fully compiled name
	return name.getFullname();
}

std::string Namespace::makeUnique(const std::string& originalName) {
	// Create the maintainable object out of the given full name
	ComplexName name(originalName);

	// Make this name unique
	_uniqueNames.makeUnique(name);

	// Return the fully compiled name
	return name.getFullname();
}

Namespace::~Namespace() {
	assert(_observers.empty());
	assert(_uniqueNames.empty());
}

void Namespace::addNameObserver(const std::string& name, NameObserver& observer) {
	// Just insert the observer
	_observers.insert(ObserverMap::value_type(name, &observer));
}

void Namespace::removeNameObserver(const std::string& name, NameObserver& observer) {
	// Lookup the iterator boundaries and find the observer
	for (ObserverMap::iterator i = _observers.lower_bound(name), upperBound = _observers.upper_bound(name); 
		 i != _observers.end() && i != upperBound; i++)
	{
		if (i->second == &observer) {
			_observers.erase(i);
			break;
		}
	}
}

void Namespace::nameChanged(const std::string& oldName, const std::string& newName) {
	// Check if we should do anything at all
	if (oldName == newName) {
		return;
	}

	// Remove the name from our UniqueNameSet
	if (!_uniqueNames.erase(oldName)) {
		globalErrorStream() << "[Namespace]: Could not remove old name before rename: " << oldName << "\n";
	}

	// Insert the new name, the NameObservers expect the new name to be present in the namespace
	_uniqueNames.insert(newName);

	// Notify the observers

	// greebo: Note, we compare the name after checking upper_bound() once more to catch cases
	// where a name gets changed from path_2 to path_23 for instance, which could result in a new
	// upper_bound(). The operator!= subsequently fails as the iterator points to the element
	// *after* the new upper_bound(), and the wrong observer gets called.
	// Performance should not be a problem as this only gets executed on name changes, and there
	// is a finite number of observers watching a single name anyway.
	
	for (ObserverMap::iterator i = _observers.lower_bound(oldName); 
		 i != _observers.end() && i != _observers.upper_bound(oldName) && i->first == oldName; 
		 /* in-loop increment */)
	{
		assert(i->second != NULL);

		// Call the observer, but increment the iterator beforehand,
		// as the observer might remove this one.
		(i++)->second->onNameChange(oldName, newName);
	}

	// greebo: usually, there are no more observers left in the multimap at this point
	// as the default observing classes remove themselves from the namespace when the
	// name changes. However, it's possible that there are some observers left,
	// so we need to redirect them to the new name.

	// This list will temporarily hold observers, as we need to change
	// their association after the name change.
	std::list<NameObserver*> temp;

	// Now go through that list again, and rename all remaining observers which
	// point at the old name (ideally, there are none left at this point)
	for (ObserverMap::iterator i = _observers.lower_bound(oldName); 
		 i != _observers.end() && i != _observers.upper_bound(oldName); 
		 /* in-loop increment */)
	{
		temp.push_back(i->second);
		_observers.erase(i++);
	}

	// greebo: Now, associate each observer with the new name (it has changed after all)
	for (std::list<NameObserver*>::iterator i = temp.begin(); i != temp.end(); i++)
	{
		_observers.insert(ObserverMap::value_type(newName, *i));
	}
}

void Namespace::importNames(const scene::INodePtr& root) {
	// Instantiate a new, temporary namespace for the nodes below root
	Namespace foreignNamespace;

	// Move all nodes below (and including) root into this temporary namespace
	foreignNamespace.connect(root);
	
	// Collect all namespaced items from the foreign root
	std::set<NamespacedPtr> importNodes;

	GatherNamespacedWalker walker(importNodes);
	Node_traverseSubgraph(root, walker);

	// Copy the existing nameset, we need a temporary one to combine the
	// existing names with the imported names without altering this Namespace.
	UniqueNameSet combinedNameSet = _uniqueNames;

	// greebo: Merge all existing names from the foreign namespace into the combined.
	// We need to know all existing names to ensure that newly created names
	// are unique in *both* namespaces
	combinedNameSet.merge(foreignNamespace._uniqueNames);

	// Now ensure that all import candidates are renamed to fit into 
	// this Namespace without conflicts
	for (std::set<NamespacedPtr>::iterator i = importNodes.begin(); 
		 i != importNodes.end(); i++)
	{
		Namespaced& namespaced = *(*i);

		// Get the name and analyse it by converting it to a complex name object
		std::string importName = namespaced.getName();
		ComplexName importComplexName(importName);

		if (combinedNameSet.nameExists(importName)) {
			// Name exists in the target namespace, get a new name
			combinedNameSet.makeUniqueAndInsert(importComplexName);

			globalOutputStream() << "Name: " << importName << " already exists in this namespace. Rename it to " 
				<< importComplexName.getFullname() << "\n";

			// Change the name of the imported node, this should
			// trigger all observers in the foreign namespace
			namespaced.changeName(importComplexName.getFullname());
		}
		else {
			globalOutputStream() << "Name: " << importName << " is fine, can be imported.\n";

			// Name does not exist yet, insert it
			combinedNameSet.insert(importComplexName);
		}
	}

	// at this point, all names in the foreign namespace have been converted to 
	// something unique in this namespace. The calling code can now move the
	// nodes into this namespace without name conflicts

	// Disconnect the root from the foreign namespace again, it will be destroyed now
	foreignNamespace.disconnect(root);
}
