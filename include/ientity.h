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

#include "generic/constant.h"
#include "generic/callbackfwd.h"
#include <string>
#include <boost/shared_ptr.hpp>

class IEntityClass;
typedef boost::shared_ptr<IEntityClass> IEntityClassPtr;
typedef boost::shared_ptr<const IEntityClass> IEntityClassConstPtr;

typedef Callback1<const std::string&> KeyObserver;

class EntityKeyValue
{
public:
  virtual const char* c_str() const = 0;
  virtual void assign(const std::string& other) = 0;
  virtual void attach(const KeyObserver& observer) = 0;
  virtual void detach(const KeyObserver& observer) = 0;
};

class Entity
{
public:
  STRING_CONSTANT(Name, "Entity");

  class Observer
  {
  public:
    virtual void insert(const char* key, EntityKeyValue& value) = 0;
    virtual void erase(const char* key, EntityKeyValue& value) = 0;
    virtual void clear() { };
  };

	/**
	 * Visitor class for keyvalues on an entity.
	 */
	struct Visitor 
	{
		// Visit function called for each key/value pair
    	virtual void visit(const std::string& key, 
    					   const std::string& value) = 0;
	};

	/**
	 * Return the entity class object for this entity.
	 */
	virtual IEntityClassConstPtr getEntityClass() const = 0;
  
	/**
	 * Enumerate key values on this entity using a Entity::Visitor class.
	 */
	virtual void forEachKeyValue(Visitor& visitor) const = 0;

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
	
  virtual bool isContainer() const = 0;
  virtual void attach(Observer& observer) = 0;
  virtual void detach(Observer& observer) = 0;
};

class EntityNode
{
public:
	/** greebo: Temporary workaround for entity-containing nodes.
	 * 			This is only used by Node_getEntity to retrieve the 
	 * 			contained entity from a node.
	 */
	virtual Entity& getEntity() = 0;
};

class EntityCopyingVisitor : public Entity::Visitor
{
  Entity& m_entity;
public:
  EntityCopyingVisitor(Entity& entity)
    : m_entity(entity)
  {
  }
	
	// Required visit function, copies keyvalues (except classname) between
	// entities
	void visit(const std::string& key, const std::string& value) {
		if(key != "classname") {
			m_entity.setKeyValue(key, value);
		}
	}
};

template<typename value_type>
class Stack;
template<typename Contained>
class Reference;

namespace scene
{
  class Node;
}

typedef Reference<scene::Node> NodeReference;

namespace scene
{
  typedef Stack<NodeReference> Path;
}

class Counter;

class EntityCreator
{
public:
  INTEGER_CONSTANT(Version, 2);
  STRING_CONSTANT(Name, "entity");

  virtual scene::Node& createEntity(IEntityClassPtr eclass) = 0;

  typedef void (*KeyValueChangedFunc)();
  virtual void setKeyValueChangedFunc(KeyValueChangedFunc func) = 0;

  virtual void setCounter(Counter* counter) = 0;

  virtual void connectEntities(const scene::Path& e1, const scene::Path& e2) = 0;

  virtual void printStatistics() const = 0;
};

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<EntityCreator> GlobalEntityModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<EntityCreator> GlobalEntityModuleRef;

inline EntityCreator& GlobalEntityCreator()
{
  return GlobalEntityModule::getTable();
}

#endif
