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

#if !defined (INCLUDED_ENTITYLIB_H)
#define INCLUDED_ENTITYLIB_H

#include "ireference.h"
#include "debugging/debugging.h"

#include "ientity.h"
#include "ieclass.h"
#include "irender.h"
#include "igl.h"
#include "selectable.h"

#include "generic/callback.h"
#include "math/aabb.h"
#include "undolib.h"
#include "string/pooledstring.h"
#include "generic/referencecounted.h"
#include "scenelib.h"
#include "container/container.h"

#include <list>
#include <set>

/* greebo: draws a pyramid defined by 5 vertices
 * points[0] is the top of the pyramid
 * points[1] to points[4] is the base rectangle
 */
inline void drawPyramid(const Vector3 points[5]) {
  typedef std::size_t index_t;
  index_t indices[16] = {
    0, 1, // top to first
    0, 2, // top to second
    0, 3, // top to third
    0, 4, // top to fourth
    1, 2, // first to second
    2, 3, // second to third
    3, 4, // third to second
    4, 1, // fourth to first 
  };
  glVertexPointer(3, GL_DOUBLE, 0, points);
  glDrawElements(GL_LINES, sizeof(indices)/sizeof(index_t), GL_UNSIGNED_INT, indices);
}

/* greebo: draws a frustum defined by 8 vertices
 * points[0] to points[3] define the top area vertices (clockwise starting from the "upper right" corner)
 * points[4] to points[7] define the base rectangle (clockwise starting from the "upper right" corner)
 */
inline void drawFrustum(const Vector3 points[8]) {
  typedef std::size_t index_t;
  index_t indices[24] = {
  	0, 4, // top up right to bottom up right
  	1, 5, // top down right to bottom down right
  	2, 6, // top down left to bottom down left
  	3, 7, // top up left to bottom up left
  	
  	0, 1, // top up right to top down right
  	1, 2, // top down right to top down left
  	2, 3, // top down left to top up left
  	3, 0, // top up left to top up right
  	
  	4, 5, // bottom up right to bottom down right
  	5, 6, // bottom down right to bottom down left
  	6, 7, // bottom down left to bottom up left
  	7, 4, // bottom up left to bottom up right
  };
  glVertexPointer(3, GL_DOUBLE, 0, points);
  glDrawElements(GL_LINES, sizeof(indices)/sizeof(index_t), GL_UNSIGNED_INT, indices);
}

inline void arrow_draw(const Vector3& origin, const Vector3& direction)
{
  Vector3 up(0, 0, 1);
  Vector3 left(-direction[1], direction[0], 0);

	Vector3 endpoint(origin + direction*32.0);

  Vector3 tip1(endpoint + direction *(-8.0) + up*(-4.0));
	Vector3 tip2(tip1 + up*8.0);
  Vector3 tip3(endpoint + direction*(-8.0) + left*(-4.0));
	Vector3 tip4(tip3 + left*8.0);

  glBegin (GL_LINES);

  glVertex3dv(origin);
  glVertex3dv(endpoint);

  glVertex3dv(endpoint);
  glVertex3dv(tip1);

  glVertex3dv(endpoint);
  glVertex3dv(tip2);

  glVertex3dv(endpoint);
  glVertex3dv(tip3);

  glVertex3dv(endpoint);
  glVertex3dv(tip4);

  glVertex3dv(tip1);
  glVertex3dv(tip3);

  glVertex3dv(tip3);
  glVertex3dv(tip2);

  glVertex3dv(tip2);
  glVertex3dv(tip4);

  glVertex3dv(tip4);
  glVertex3dv(tip1);

  glEnd();
}

class SelectionIntersection;

inline void aabb_testselect(const AABB& aabb, SelectionTest& test, SelectionIntersection& best)
{
  const IndexPointer::index_type indices[24] = {
    2, 1, 5, 6,
    1, 0, 4, 5,
    0, 1, 2, 3,
    3, 7, 4, 0,
    3, 2, 6, 7,
    7, 6, 5, 4,
  };

  Vector3 points[8];
  aabb_corners(aabb, points);
  test.TestQuads(VertexPointer(reinterpret_cast<VertexPointer::pointer>(points), sizeof(Vector3)), IndexPointer(indices, 24), best);
}

inline void aabb_draw_wire(const Vector3 points[8])
{
  typedef std::size_t index_t;
  index_t indices[24] = {
    0, 1, 1, 2, 2, 3, 3, 0,
    4, 5, 5, 6, 6, 7, 7, 4,
    0, 4, 1, 5, 2, 6, 3, 7,
  };
#if 1
  glVertexPointer(3, GL_DOUBLE, 0, points);
  glDrawElements(GL_LINES, sizeof(indices)/sizeof(index_t), GL_UNSIGNED_INT, indices);
#else
  glBegin(GL_LINES);
  for(std::size_t i = 0; i < sizeof(indices)/sizeof(index_t); ++i)
  {
    glVertex3dv(points[indices[i]]);
  }
  glEnd();
#endif
}

inline void aabb_draw_flatshade(const Vector3 points[8])
{
  glBegin(GL_QUADS);

  glNormal3dv(aabb_normals[0]);
  glVertex3dv(points[2]);
  glVertex3dv(points[1]);
  glVertex3dv(points[5]);
  glVertex3dv(points[6]);

  glNormal3dv(aabb_normals[1]);
  glVertex3dv(points[1]);
  glVertex3dv(points[0]);
  glVertex3dv(points[4]);
  glVertex3dv(points[5]);

  glNormal3dv(aabb_normals[2]);
  glVertex3dv(points[0]);
  glVertex3dv(points[1]);
  glVertex3dv(points[2]);
  glVertex3dv(points[3]);

  glNormal3dv(aabb_normals[3]);
  glVertex3dv(points[0]);
  glVertex3dv(points[3]);
  glVertex3dv(points[7]);
  glVertex3dv(points[4]);

  glNormal3dv(aabb_normals[4]);
  glVertex3dv(points[3]);
  glVertex3dv(points[2]);
  glVertex3dv(points[6]);
  glVertex3dv(points[7]);

  glNormal3dv(aabb_normals[5]);
  glVertex3dv(points[7]);
  glVertex3dv(points[6]);
  glVertex3dv(points[5]);
  glVertex3dv(points[4]);

  glEnd();
}

inline void aabb_draw_wire(const AABB& aabb)
{
  Vector3 points[8];
	aabb_corners(aabb, points);
  aabb_draw_wire(points);
}

inline void aabb_draw_flatshade(const AABB& aabb)
{
  Vector3 points[8];
	aabb_corners(aabb, points);
  aabb_draw_flatshade(points);
}

inline void aabb_draw_textured(const AABB& aabb)
{
  Vector3 points[8];
	aabb_corners(aabb, points);

  glBegin(GL_QUADS);

  glNormal3dv(aabb_normals[0]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[2]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[1]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[5]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[6]);

  glNormal3dv(aabb_normals[1]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[1]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[0]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[4]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[5]);

  glNormal3dv(aabb_normals[2]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[0]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[1]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[2]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[3]);

  glNormal3dv(aabb_normals[3]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[0]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[3]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[7]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[4]);

  glNormal3dv(aabb_normals[4]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[3]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[2]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[6]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[7]);

  glNormal3dv(aabb_normals[5]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[7]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[6]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[5]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[4]);

  glEnd();
}

inline void aabb_draw_solid(const AABB& aabb, RenderStateFlags state)
{
  if(state & RENDER_TEXTURE)
  {
    aabb_draw_textured(aabb);
  }
  else
  {
    aabb_draw_flatshade(aabb);
  }
}

inline void aabb_draw(const AABB& aabb, RenderStateFlags state)
{
  if(state & RENDER_FILL)
  {
    aabb_draw_solid(aabb, state);
  }
  else
  {
    aabb_draw_wire(aabb);
  }
}

class RenderableSolidAABB : public OpenGLRenderable
{
  const AABB& m_aabb;
public:
  RenderableSolidAABB(const AABB& aabb) : m_aabb(aabb)
  {
  }
  void render(RenderStateFlags state) const
  {
    aabb_draw_solid(m_aabb, state);
  }
};

class RenderableWireframeAABB : public OpenGLRenderable
{
  const AABB& m_aabb;
public:
  RenderableWireframeAABB(const AABB& aabb) : m_aabb(aabb)
  {
  }
  void render(RenderStateFlags state) const
  {
    aabb_draw_wire(m_aabb);
  }
};


/// \brief A key/value pair of strings.
///
/// - Notifies observers when value changes - value changes to "" on destruction.
/// - Provides undo support through the global undo system.
class KeyValue : public EntityKeyValue
{
  typedef UnsortedSet<KeyObserver> KeyObservers;

  std::size_t m_refcount;
  KeyObservers m_observers;
  CopiedString m_string;
  const char* m_empty;
  ObservedUndoableObject<CopiedString> m_undo;
  static EntityCreator::KeyValueChangedFunc m_entityKeyValueChanged;
public:

  KeyValue(const char* string, const char* empty)
    : m_refcount(0), m_string(string), m_empty(empty), m_undo(m_string, UndoImportCaller(*this))
  {
    notify();
  }
  ~KeyValue()
  {
    ASSERT_MESSAGE(m_observers.empty(), "KeyValue::~KeyValue: observers still attached");
  }

  static void setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func)
  {
    m_entityKeyValueChanged = func;
  }

  void IncRef()
  {
    ++m_refcount;
  }
  void DecRef()
  {
    if(--m_refcount == 0)
    {
      delete this;
    }
  }

  void instanceAttach(MapFile* map)
  {
    m_undo.instanceAttach(map);
  }
  void instanceDetach(MapFile* map)
  {
    m_undo.instanceDetach(map);
  }

  void attach(const KeyObserver& observer)
  {
    (*m_observers.insert(observer))(c_str());
  }
  void detach(const KeyObserver& observer)
  {
    observer(m_empty);
    m_observers.erase(observer);
  }
  const char* c_str() const
  {
    if(string_empty(m_string.c_str()))
    {
      return m_empty;
    }
    return m_string.c_str();
  }
  void assign(const char* other)
  {
    if(!string_equal(m_string.c_str(), other))
    {
      m_undo.save();
      m_string = other;
      notify();
    }
  }

  void notify()
  {
    m_entityKeyValueChanged();
    KeyObservers::reverse_iterator i = m_observers.rbegin();
    while(i != m_observers.rend())
    {
      (*i++)(c_str());
    }
  }

  void importState(const CopiedString& string)
  {
    m_string = string;

    notify();
  }
  typedef MemberCaller1<KeyValue, const CopiedString&, &KeyValue::importState> UndoImportCaller;
};

/// \brief An unsorted list of key/value pairs.
///
/// - Notifies observers when a pair is inserted or removed.
/// - Provides undo support through the global undo system.
/// - New keys are appended to the end of the list.
class Doom3Entity : 
	public Entity
{
public:
  typedef KeyValue Value;

  static StringPool& getPool()
  {
    return Static<StringPool, KeyContext>::instance();
  }
private:
  static EntityCreator::KeyValueChangedFunc m_entityKeyValueChanged;
  static Counter* m_counter;

  IEntityClassConstPtr m_eclass;

  class KeyContext{};
  typedef Static<StringPool, KeyContext> KeyPool;
  typedef PooledString<KeyPool> Key;
  typedef SmartPointer<KeyValue> KeyValuePtr;
  typedef UnsortedMap<Key, KeyValuePtr> KeyValues;
  KeyValues m_keyValues;

  typedef UnsortedSet<Observer*> Observers;
  Observers m_observers;

  ObservedUndoableObject<KeyValues> m_undo;
  bool m_instanced;

  bool m_observerMutex;

  void notifyInsert(const char* key, Value& value)
  {
    m_observerMutex = true;
    for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i)
    {
      (*i)->insert(key, value);
    }
    m_observerMutex = false;
  }
  void notifyErase(const char* key, Value& value)
  {
    m_observerMutex = true;
    for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i)
    {
      (*i)->erase(key, value);
    }
    m_observerMutex = false;
  }
  void forEachKeyValue_notifyInsert()
  {
    for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i)
    {
      notifyInsert((*i).first.c_str(), *(*i).second);
    }
  }
  void forEachKeyValue_notifyErase()
  {
    for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i)
    {
      notifyErase((*i).first.c_str(), *(*i).second);
    }
  }

  void insert(const char* key, const KeyValuePtr& keyValue)
  {
    KeyValues::iterator i = m_keyValues.insert(KeyValues::value_type(key, keyValue));
    notifyInsert(key, *(*i).second);

    if(m_instanced)
    {
      (*i).second->instanceAttach(m_undo.map());
    }
  }

  void insert(const char* key, const char* value)
  {
    KeyValues::iterator i = m_keyValues.find(key);
    if(i != m_keyValues.end())
    {
      (*i).second->assign(value);
    }
    else
    {
      m_undo.save();
      insert(key, KeyValuePtr(new KeyValue(value, m_eclass->getValueForKey(key).c_str())));
    }
  }

  void erase(KeyValues::iterator i)
  {
    if(m_instanced)
    {
      (*i).second->instanceDetach(m_undo.map());
    }

    Key key((*i).first);
    KeyValuePtr value((*i).second);
    m_keyValues.erase(i);
    notifyErase(key.c_str(), *value);
  }

  void erase(const char* key)
  {
    KeyValues::iterator i = m_keyValues.find(key);
    if(i != m_keyValues.end())
    {
      m_undo.save();
      erase(i);
    }
  }

public:
  bool m_isContainer;

  Doom3Entity(IEntityClassPtr eclass) :
    m_eclass(eclass),
    m_undo(m_keyValues, UndoImportCaller(*this)),
    m_instanced(false),
    m_observerMutex(false),
    m_isContainer(!eclass->isFixedSize())
  {
  }
  Doom3Entity(const Doom3Entity& other) :
    Entity(other),
    m_eclass(other.getEntityClass()),
    m_undo(m_keyValues, UndoImportCaller(*this)),
    m_instanced(false),
    m_observerMutex(false),
    m_isContainer(other.m_isContainer)
  {
    for(KeyValues::const_iterator i = other.m_keyValues.begin(); i != other.m_keyValues.end(); ++i)
    {
      insert((*i).first.c_str(), (*i).second->c_str());
    }
  }
  ~Doom3Entity()
  {
    for(Observers::iterator i = m_observers.begin(); i != m_observers.end();)
    {
      // post-increment to allow current element to be removed safely
      (*i++)->clear();
    }
    ASSERT_MESSAGE(m_observers.empty(), "EntityKeyValues::~EntityKeyValues: observers still attached");
  }

  static void setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func)
  {
    m_entityKeyValueChanged = func;
    KeyValue::setKeyValueChangedFunc(func);
  }
  static void setCounter(Counter* counter)
  {
    m_counter = counter;
  }

  void importState(const KeyValues& keyValues)
  {
    for(KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end();)
    {
      erase(i++);
    }

    for(KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i)
    {
      insert((*i).first.c_str(), (*i).second);
    }

    m_entityKeyValueChanged();
  }
  typedef MemberCaller1<Doom3Entity, const KeyValues&, &Doom3Entity::importState> UndoImportCaller;

  void attach(Observer& observer)
  {
    ASSERT_MESSAGE(!m_observerMutex, "observer cannot be attached during iteration");
    m_observers.insert(&observer);
    for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i)
    {
      observer.insert((*i).first.c_str(), *(*i).second);
    }
  }
  void detach(Observer& observer)
  {
    ASSERT_MESSAGE(!m_observerMutex, "observer cannot be detached during iteration");
    m_observers.erase(&observer);
    for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i)
    {
      observer.erase((*i).first.c_str(), *(*i).second);
    }
  }

  void forEachKeyValue_instanceAttach(MapFile* map)
  {
    for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i)
    {
      (*i).second->instanceAttach(map);
    }
  }
  void forEachKeyValue_instanceDetach(MapFile* map)
  {
    for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i)
    {
      (*i).second->instanceDetach(map);
    }
  }

  void instanceAttach(MapFile* map)
  {
    if(m_counter != 0)
    {
      m_counter->increment();
    }

    m_instanced = true;
    forEachKeyValue_instanceAttach(map);
    m_undo.instanceAttach(map);
  }
  void instanceDetach(MapFile* map)
  {
    if(m_counter != 0)
    {
      m_counter->decrement();
    }

    m_undo.instanceDetach(map);
    forEachKeyValue_instanceDetach(map);
    m_instanced = false;
  }

	/** Return the EntityClass associated with this entity.
	 */
	IEntityClassConstPtr getEntityClass() const {
		return m_eclass;
	}
	
  void forEachKeyValue(Visitor& visitor) const
  {
    for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i)
    {
      visitor.visit((*i).first.c_str(), (*i).second->c_str());
    }
  }
  
	/** Set a keyvalue on the entity. 
	 */
	void setKeyValue(const std::string& key, const std::string& value) {
		if (value.empty()) {
			erase(key.c_str());
		}
		else {
			insert(key.c_str(), value.c_str());
		}
		m_entityKeyValueChanged();
	}
  
	/** Retrieve a keyvalue from the entity.
	 */
	std::string getKeyValue(const std::string& key) const {

		// Lookup the key in the map
		KeyValues::const_iterator i = m_keyValues.find(key.c_str());

		// If key is found, return it, otherwise lookup the default value on
		// the entity class
		if(i != m_keyValues.end()) {
			return i->second->c_str();
		}
		else {
			return m_eclass->getValueForKey(key);
		}
	}

	bool isContainer() const {
		return m_isContainer;
	}
};

/// \brief A Resource reference with a controlled lifetime.
/// \brief The resource is released when the ResourceReference is destroyed.
// TODO: Deprecated, use ResourcePtr instead
class ResourceReference
{
  std::string m_name;
  ReferenceCache::ResourcePtr m_resource;
public:
  ResourceReference(const char* name)
    : m_name(name)
  {
    capture();
  }
  ResourceReference(const ResourceReference& other)
    : m_name(other.m_name)
  {
    capture();
  }
  ResourceReference& operator=(const ResourceReference& other)
  {
    ResourceReference tmp(other);
    tmp.swap(*this);
    return *this;
  }
  ~ResourceReference()
  {
    release();
  }

  void capture()
  {
    m_resource = GlobalReferenceCache().capture(m_name);
  }
  void release()
  {
    GlobalReferenceCache().release(m_name);
  }

  const char* getName() const
  {
    return m_name.c_str();
  }
  void setName(const char* name)
  {
    ResourceReference tmp(name);
    tmp.swap(*this);
  }

  void swap(ResourceReference& other)
  {
    std::swap(m_resource, other.m_resource);
    std::swap(m_name, other.m_name);
  }

  void attach(ModuleObserver& observer)
  {
    m_resource->attach(observer);
  }
  void detach(ModuleObserver& observer)
  {
    m_resource->detach(observer);
  }

  ReferenceCache::ResourcePtr get()
  {
    return m_resource;
  }
};

namespace std
{
  /// \brief Swaps the values of \p self and \p other.
  /// Overloads std::swap.
  inline void swap(ResourceReference& self, ResourceReference& other)
  {
    self.swap(other);
  }
}

/** Walker to locate an Entity in the scenegraph with a specific classname.
 */
class EntityFindByClassnameWalker : 
	public scene::Graph::Walker
{
	// Name to search for
	std::string _name;
	
	// Reference to a pointer to modify with the result 
	mutable Entity* _entity;
	
public:
	// Constructor
	EntityFindByClassnameWalker(const std::string& name) : 
		_name(name),
		_entity(NULL)
	{}
	
	Entity* getEntity() {
		return _entity;
	}
	
	// Pre-descent callback
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if (_entity == NULL) {
			// Entity not found yet
			
			Entity* entity = Node_getEntity(path.top());
			
			if(entity != NULL  && _name == entity->getKeyValue("classname")) {
				_entity = entity;
			}
		}
		return true;
	}
};

#endif
