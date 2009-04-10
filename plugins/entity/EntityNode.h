#ifndef _ENTITY_NODE_H_
#define _ENTITY_NODE_H_

#include "ientity.h"
#include "inamespace.h"
#include "NamespaceManager.h"

namespace entity {

/**
 * greebo: This is the common base class of all map entities.
 */
class EntityNode :
	public IEntityNode,
	public Namespaced
{
protected:
	// The entity class
	IEntityClassConstPtr _eclass;

	// The actual entity (which contains the key/value pairs)
	// TODO: Rename this to "spawnargs"?
	Doom3Entity _entity;

	// The class taking care of all the namespace-relevant stuff
	NamespaceManager _namespaceManager;

public:
	// The Constructor needs the eclass
	EntityNode(const IEntityClassConstPtr& eclass);

	// Copy constructor
	EntityNode(const EntityNode& other);

	// Namespaced implementation
	// Gets/sets the namespace of this named object
	std::string getName();
	void setNamespace(INamespace* space);
	INamespace* getNamespace() const;
	void connectNameObservers();
	void disconnectNameObservers();
	void changeName(const std::string& newName);

	void attachNames();
	void detachNames();
};

} // namespace entity

#endif /* _ENTITY_NODE_H_ */
