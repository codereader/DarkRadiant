#ifndef _ENTITY_NODE_H_
#define _ENTITY_NODE_H_

#include "ientity.h"
#include "inamespace.h"
#include "Bounded.h"

#include "selectionlib.h"
#include "transformlib.h"

#include "NamespaceManager.h"
#include "target/TargetableNode.h"
#include "NameKey.h"

#include "KeyObserverMap.h"

namespace entity {

/**
 * greebo: This is the common base class of all map entities.
 */
class EntityNode :
	public IEntityNode,
	public SelectableNode, // derives from scene::Node
	public Namespaced,
	public TargetableNode,
	public Renderable,
	public Nameable,
	public Transformable,
	public MatrixTransform,	// influences local2world of child nodes
	public scene::Cloneable, // all entities are cloneable, to be implemented in subclasses
	public IEntityClass::Observer
{
protected:
	// The entity class
	IEntityClassPtr _eclass;

	// The actual entity (which contains the key/value pairs)
	// TODO: Rename this to "spawnargs"?
	Doom3Entity _entity;

	// The class taking care of all the namespace-relevant stuff
	NamespaceManager _namespaceManager;

	// A helper class observing the "name" keyvalue
	// Used for rendering the name and as Nameable implementation
	NameKey _nameKey;

	// The OpenGLRenderable, using the NameKey helper class to retrieve the name
	RenderableNameKey _renderableName;

	// A helper class managing the collection of KeyObservers attached to the Doom3Entity
	KeyObserverMap _keyObservers;

public:
	// The Constructor needs the eclass
	EntityNode(const IEntityClassPtr& eclass);

	// Copy constructor
	EntityNode(const EntityNode& other);

	virtual ~EntityNode();

	// IEntityNode implementation
	Entity& getEntity();

	// Namespaced implementation
	// Gets/sets the namespace of this named object
	std::string getName() const;
	void setNamespace(INamespace* space);
	INamespace* getNamespace() const;
	void connectNameObservers();
	void disconnectNameObservers();
	void changeName(const std::string& newName);

	void attachNames();
	void detachNames();

	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

	// Nameable implementation
	virtual std::string name() const;

	// Renderable implementation, can be overridden by subclasses
	virtual void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	virtual void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	// Adds/removes the keyobserver to/from the KeyObserverMap
	void addKeyObserver(const std::string& key, KeyObserver& observer);
	void removeKeyObserver(const std::string& key, KeyObserver& observer);

	virtual void OnEClassReload();

private:
	// Routines used by constructor and destructor, should be non-virtual
	void construct();
	void destruct();
};

} // namespace entity

#endif /* _ENTITY_NODE_H_ */
