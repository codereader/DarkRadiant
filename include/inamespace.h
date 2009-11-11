#ifndef _INAMESPACE_H__
#define _INAMESPACE_H__

#include "inode.h"
#include "imodule.h"

// Forward declaration (is declared in inameobserver.h)
class NameObserver;

/**
 * greebo: The Namespace is the managing class which keeps track
 * of all the names in the map. It gets notified on name changes
 * and broadcasts this event to all registered NameObservers.
 *
 * Additionally, it provides algorithms to merge elements from
 * another namespace into this one, preserving all links between
 * Namespaced objects.
 */
class INamespace
{
public:
    /**
	 * Destructor
	 */
	virtual ~INamespace() {}

	/** 
	 * greebo: Call this to put the given scene::Node and all its
	 * children into this Namespace. This will search for Namespaced
	 * items and tell them to add their relevant "names" to this space.
	 */
	virtual void connect(const scene::INodePtr& root) = 0;

	/** 
	 * greebo: The counter-part of the connect() method above. This will
	 * remove the given node and all its children from this namespace.
	 */
	virtual void disconnect(const scene::INodePtr& root) = 0;

	/** 
	 * greebo: Returns true if the given name already exists in this namespace.
	 */
	virtual bool nameExists(const std::string& name) = 0;

	/**
	 * greebo: Inserts the given name into this namespace. The name
	 * must not exist yet, it won't get inserted otherwise.
	 *
	 * @returns: TRUE if the insertion was successful, FALSE if the name already exists.
	 */
	virtual bool insert(const std::string& name) = 0;

	/**
	 * greebo: Removes the given name from this namespace. The name
	 * is available again afterwards.
	 *
	 * @returns: TRUE if the removal was successful, FALSE if the name did not exist.
	 */
	virtual bool erase(const std::string& name) = 0;

	/**
	 * greebo: Returns a new, currently unused name based on the given one.
	 * Usually, this means that the name gets a number appended or
	 * (if a trailing number is already present) the number is changed.
	 *
	 * After this call, the name is automatically registered in this namespace.
	 *
	 * @returns: The new name, unique in this namespace.
	 */
	virtual std::string makeUniqueAndInsert(const std::string& originalName) = 0;

	/**
	 * greebo: Returns a new, currently unused name based on the given one.
	 * Usually, this means that the name gets a number appended or
	 * (if a trailing number is already present) the number is changed.
	 *
	 * @returns: The new name, unique in this namespace.
	 */
	virtual std::string makeUnique(const std::string& originalName) = 0;

	/**
	 * greebo: Registers/de-registers a Observer. The Observer will get notified
	 * as soon as the name changes.
	 */
	virtual void addNameObserver(const std::string& name, NameObserver& observer) = 0;
	virtual void removeNameObserver(const std::string& name, NameObserver& observer) = 0;

	/** 
	 * greebo: Tells the namespace to notify the nameobservers about a name change. 
	 * This usually gets called by the entity itself, whose name has been altered.
	 */
	virtual void nameChanged(const std::string& oldName, const std::string& newName) = 0;

	/**
	 * greebo: Imports all names in the graph below (and including) <root> into
	 * this namespace. The routine will change all imported names to something
	 * unique in this namespace, so that no duplicated names occur.
	 *
	 * The nodes below <root> should have been added to a different namespace 
	 * prior to this call, so that links are preserved during name changes.
	 *
	 * After this call, the imported nodes are renamed to fit into this 
	 * namespace and can be safely connected to this Namespace.
	 */
	virtual void importNames(const scene::INodePtr& root) = 0;
};
typedef boost::shared_ptr<INamespace> INamespacePtr;

/**
 * greebo: A Namespaced object is the primary communication
 * partner for the Namespace. This object provides methods
 * to connect/disconnect items from a map Namespace.
 *
 * Note: the standard implementation in D3 maps is 
 * the NamespaceManager class in the entity module.
 *
 * Note: I chose to use raw INamespace* pointers in the interface 
 * to allow the Namespace calling these functions without maintaining
 * weak_ptrs to itself. This can of course be changed in the
 * future, but I didn't want to bother with "self" weak_ptrs right now.
 */
class Namespaced
{
public:
    /**
	 * Destructor
	 */
	virtual ~Namespaced() {}

	/**
	 * Returns the name of this object.
	 */
	virtual std::string getName() const = 0;

	// Gets/sets the namespace of this named object
	virtual void setNamespace(INamespace* space) = 0;
	virtual INamespace* getNamespace() const = 0;

	/**
	 * Tells the Namespaced object to register all relevant
	 * names with the associated Namespace.
	 */
	virtual void attachNames() = 0;

	// Counter-part of the registerNames() method above.
	virtual void detachNames() = 0;

	/** 
	 * Tells the object to change its name. Invoked by the 
	 * INamespace class itself during import.
	 */
	virtual void changeName(const std::string& newName) = 0;

	virtual void connectNameObservers() = 0;
	virtual void disconnectNameObservers() = 0;
};
typedef boost::shared_ptr<Namespaced> NamespacedPtr;

inline NamespacedPtr Node_getNamespaced(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<Namespaced>(node);
}

/**
 * greebo: Use the namespace factory to create new Namespace objects.
 */
class INamespaceFactory :
	public RegisterableModule
{
public:
	/** 
	 * Creates and returns a new Namespace.
	 */
	virtual INamespacePtr createNamespace() = 0;
};

const std::string MODULE_NAMESPACE_FACTORY("NamespaceFactory");

// Factory accessor
inline INamespaceFactory& GlobalNamespaceFactory() {
	// Cache the reference locally
	static INamespaceFactory& _namespaceFactory(
		*boost::static_pointer_cast<INamespaceFactory>(
			module::GlobalModuleRegistry().getModule(MODULE_NAMESPACE_FACTORY)
		)
	);
	return _namespaceFactory;
}

#endif /* _INAMESPACE_H__ */
