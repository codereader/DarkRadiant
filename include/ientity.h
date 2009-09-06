/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_IENTITY_H)
#define INCLUDED_IENTITY_H

#include "inode.h"
#include "ipath.h"
#include "imodule.h"
#include "inameobserver.h"
#include "generic/callbackfwd.h"

class IEntityClass;
typedef boost::shared_ptr<IEntityClass> IEntityClassPtr;
typedef boost::shared_ptr<const IEntityClass> IEntityClassConstPtr;

typedef Callback1<const std::string&> KeyObserver;

class EntityKeyValue :
	public NameObserver
{
public:
	/** greebo: Retrieves the actual value of this key
	 */
	virtual const std::string& get() const = 0;
	
	/** greebo: Sets the value of this key
	 */
	virtual void assign(const std::string& other) = 0;
	
	/** greebo: Attaches/detaches a callback to get notified about
	 * 			the key change.
	 */
	virtual void attach(const KeyObserver& observer) = 0;
	virtual void detach(const KeyObserver& observer) = 0;
};

/**
 * Interface for a map entity. The Entity is the main building block of a
 * map, and the uppermost layer in the scenegraph under the root node. Each
 * entity contains a arbitrary dictionary of strings ("properties" or 
 * "spawnargs") containing information about this entity which is used by the
 * game engine to modify its behaviour, and may additionally contain child
 * primitives (brushes and patches) depending on its type.
 * 
 * At the minimum, each Entity must contain three properties: "name" which 
 * contains a map-unique string identifier, "classname" which identifies the
 * entity class to the game, and "origin" which stores the location of the 
 * entity in 3-dimensional world space.
 * 
 * A valid <b>Id Tech 4</b> map must contain at least one entity: the
 * "worldspawn" which is the parent of all map geometry primitives. 
 */
class Entity
{
public:
	// A container with key => value pairs
	typedef std::vector< std::pair<std::string, std::string> > KeyValuePairs;

    /**
     * \brief
     * Abstract base class for entity observers.
     *
     * An entity observer receives notifications when keyvalues are inserted or
     * deleted on the entity it is observing.
     */
	class Observer
	{
	public:

        /**
         * \brief
         * Notification that a new key value has been inserted on the entity.
         */
		virtual void onKeyInsert(const std::string& key, EntityKeyValue& value)
        { }
		
        /**
         * \brief
         * Notification that a key value has changed on the entity.
         */
        virtual void onKeyChange(const std::string& key, const std::string& val)
        { }

        /**
         * \brief
         * Notification that a key value has been removed from the entity.
         */
		virtual void onKeyErase(const std::string& key, EntityKeyValue& value)
        { }
	};

	/**
	 * Visitor class for keyvalues on an entity. An Entity::Visitor is provided
	 * to an Entity via the Entity::forEachKeyValue() method, after which the
	 * visitor's visit() method will be invoked for each keyvalue on the
	 * Entity.
	 */
	struct Visitor 
	{
		/**
		 * The visit function which must be implemented by subclasses.
		 * 
		 * @param key
		 * The current key being visited.
		 * 
		 * @param value
		 * The value associated with the current key.
		 */
    	virtual void visit(const std::string& key, 
    					   const std::string& value) = 0;
	};

	/**
	 * Visitor class for keyvalues on an entity. An Entity::KeyValueVisitor is provided
	 * to an Entity via the Entity::forEachKeyValue() method, after which the
	 * visitor's visit() method will be invoked for each keyvalue on the Entity.
	 */
	struct KeyValueVisitor 
	{
		/**
		 * The visit function which must be implemented by subclasses.
		 * 
		 * @param key
		 * The current key being visited.
		 * 
		 * @param value
		 * The actual keyvalue object associated with the current key.
		 */
    	virtual void visit(const std::string& key, 
    					   EntityKeyValue& value) = 0;
	};

	/**
	 * Return the entity class object for this entity.
	 */
	virtual IEntityClassPtr getEntityClass() const = 0;
  
	/**
	 * Enumerate key values on this entity using a Entity::Visitor class.
	 */
	virtual void forEachKeyValue(Visitor& visitor) const = 0;

	// Same as above, but this one is visiting the KeyValues itself, not just strings.
	virtual void forEachKeyValue(KeyValueVisitor& visitor) = 0;

	/** Set a key value on this entity. Setting the value to "" will
	 * remove the key.
	 * 
	 * @param key
	 * The key to set.
	 * 
	 * @param value
	 * Value to give the key, or the empty string to remove the key.
	 */
	virtual void setKeyValue(const std::string& key, 
							 const std::string& value) = 0;

	/* Retrieve a key value from the entity.
	 * 
	 * @param key
	 * The key to retrieve.
	 * 
	 * @returns
	 * The current value for this key, or the empty string if it does not 
	 * exist.
	 */
	virtual std::string getKeyValue(const std::string& key) const = 0;

	/**
	 * greebo: Checks whether the given key is inherited or not.
	 *
	 * @returns: TRUE if the value is inherited, 
	 *           FALSE when it is not or when the key doesn't exist at all.
	 */
	virtual bool isInherited(const std::string& key) const = 0;

	/**
	 * Return the list of Key/Value pairs matching the given prefix.
	 * 
	 * This method performs a search for all spawnargs whose key
	 * matches the given prefix, with a suffix consisting of zero or more 
	 * arbitrary characters. For example, if "target" were specified as the
	 * prefix, the list would include "target", "target0", "target127" etc.
	 * 
	 * This operation may not have high performance, due to the need to scan
	 * for matching names, therefore should not be used in performance-critical
	 * code.
	 * 
	 * @param prefix
	 * The prefix to search for.
	 * 
	 * @return
	 * A list of KeyValue pairs matching the provided prefix. This
	 * list will be empty if there were no matches.
	 */
	virtual KeyValuePairs getKeyValuePairs(const std::string& prefix) const = 0;
	
	/** greebo: Returns true if the entity is a model. For Doom3, this is 
	 * 			usually true when the classname == "func_static" and
	 * 			the non-empty spawnarg "model" != "name".
	 */
	virtual bool isModel() const = 0;
	
  virtual bool isContainer() const = 0;

    /**
     * \brief
     * Attach an Entity::Observer to this Entity.
     */
    virtual void attachObserver(Observer* observer) = 0;

    /**
     * \brief
     * Detach an Entity::Observer from this Entity.
     */
    virtual void detachObserver(Observer* observer) = 0;
};

class IEntityNode
{
public:
	/** greebo: Temporary workaround for entity-containing nodes.
	 * 			This is only used by Node_getEntity to retrieve the 
	 * 			contained entity from a node.
	 */
	virtual Entity& getEntity() = 0;

	/**
	 * greebo: Tells the entity to reload the child model. This usually
	 *         includes removal of the child model node and triggering
	 *         a "skin changed" event.
	 */
	virtual void refreshModel() = 0;
};
typedef boost::shared_ptr<IEntityNode> IEntityNodePtr; 

inline Entity* Node_getEntity(scene::INodePtr node) {
	IEntityNodePtr entityNode = boost::dynamic_pointer_cast<IEntityNode>(node);
	if (entityNode != NULL) {
		return &(entityNode->getEntity());
	}
	return NULL;
}

inline bool Node_isEntity(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<IEntityNode>(node) != NULL;
}

/**
 * greebo: This is a visitor class copying all visited spawnargs to
 *         the target entity passed to the constructor (except classname).
 */ 
class EntityCopyingVisitor : 
	public Entity::Visitor
{
	// the target entity
	Entity& _entity; 
public:
	EntityCopyingVisitor(Entity& entity) : 
		_entity(entity)
	{}
	
	// Required visit function, copies keyvalues (except classname) between
	// entities
	void visit(const std::string& key, const std::string& value) {
		if (key != "classname") {
			_entity.setKeyValue(key, value);
		}
	}
};

const std::string MODULE_ENTITYCREATOR("Doom3EntityCreator");

/**
 * \brief
 * Interface for the entity creator module.
 */
class EntityCreator :
	public RegisterableModule
{
public:

    /**
     * \brief
     * Create an entity node with the given entity class.
     */
    virtual scene::INodePtr createEntity(const IEntityClassPtr& eclass) = 0;

    /**
     * \brief
     * Connect the two given entity nodes using the "target" system.
     */
	virtual void connectEntities(const scene::INodePtr& source,
                                 const scene::INodePtr& target) = 0;
};

inline EntityCreator& GlobalEntityCreator() {
	// Cache the reference locally
	static EntityCreator& _entityCreator(
		*boost::static_pointer_cast<EntityCreator>(
			module::GlobalModuleRegistry().getModule(MODULE_ENTITYCREATOR)
		)
	);
	return _entityCreator;
}

#endif
