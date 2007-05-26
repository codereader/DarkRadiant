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

#if !defined (INCLUDED_MAPLIB_H)
#define INCLUDED_MAPLIB_H

#include "nameable.h"
#include "mapfile.h"

#include "traverselib.h"
#include "transformlib.h"
#include "scenelib.h"
#include "string/string.h"
#include "instancelib.h"
#include "selectionlib.h"
#include "generic/callback.h"

#include <string>

class UndoFileChangeTracker : public UndoTracker, public MapFile
{
  std::size_t m_size;
  std::size_t m_saved;
  typedef void (UndoFileChangeTracker::*Pending)();
  Pending m_pending;
  Callback m_changed;

public:
  UndoFileChangeTracker() : m_size(0), m_saved(MAPFILE_MAX_CHANGES), m_pending(0)
  {
  }
  void print()
  {
    globalOutputStream() << "saved: " << Unsigned(m_saved) << " size: " << Unsigned(m_size) << "\n";
  }

  void push()
  {
    ++m_size;
    m_changed();
    //print();
  }
  void pop()
  {
    --m_size;
    m_changed();
    //print();
  }
  void pushOperation()
  {
    if(m_size < m_saved)
    {
      // redo queue has been flushed.. it is now impossible to get back to the saved state via undo/redo
      m_saved = MAPFILE_MAX_CHANGES;
    }
    push();
  }
  void clear()
  {
    m_size = 0;
    m_changed();
    //print();
  }
  void begin()
  {
    m_pending = Pending(&UndoFileChangeTracker::pushOperation);
  }
  void undo()
  {
    m_pending = Pending(&UndoFileChangeTracker::pop);
  }
  void redo()
  {
    m_pending = Pending(&UndoFileChangeTracker::push);
  }

  void changed()
  {
    if(m_pending != 0)
    {
      ((*this).*m_pending)();
      m_pending = 0;
    }
  }

  void save()
  {
    m_saved = m_size;
    m_changed();
  }
  bool saved() const
  {
    return m_saved == m_size;
  }

  void setChangedCallback(const Callback& changed)
  {
    m_changed = changed;
    m_changed();
  }

  std::size_t changes() const
  {
    return m_size;
  }
};


class MapRoot : 
	public scene::Node, 
	public scene::Instantiable, 
	public scene::Traversable::Observer,
	public Nameable,
	public TransformNode,
	public MapFile,
	public scene::Traversable
{
  IdentityTransform m_transform;
  TraversableNodeSet m_traverse;
  InstanceSet m_instances;
  typedef SelectableInstance Instance;
	std::string _name;
  UndoFileChangeTracker m_changeTracker;
public:
  	// scene::Traversable Implementation
	virtual void insert(scene::INodePtr node) {
		m_traverse.insert(node);
	}
    virtual void erase(scene::INodePtr node) {
    	m_traverse.erase(node);	
    }
    virtual void traverse(const Walker& walker) {
    	m_traverse.traverse(walker);
    }
    virtual bool empty() const {
    	return m_traverse.empty();
    }
	
	// TransformNode implementation
	virtual const Matrix4& localToParent() const {
		return m_transform.localToParent();
	}
  
	// MapFile implementation
	virtual void save() {
		m_changeTracker.save();
	}
	virtual bool saved() const {
		return m_changeTracker.saved();
	}
	virtual void changed() {
		m_changeTracker.changed();
	}
	virtual void setChangedCallback(const Callback& changed) {
		m_changeTracker.setChangedCallback(changed);
	}
	virtual std::size_t changes() const {
		return m_changeTracker.changes();
	}

	MapRoot(const std::string& name) : 
		_name(name)
	{
		// Set this node to root status
		setIsRoot(true);
		m_traverse.attach(this);
	
		GlobalUndoSystem().trackerAttach(m_changeTracker);
	}

	std::string name() const {
		return _name;
	}
	
	void attach(const NameCallback& callback)
	{}
  
	void detach(const NameCallback& callback)
	{}

  virtual ~MapRoot() {
  	// Override the default release() method
    GlobalUndoSystem().trackerDetach(m_changeTracker);

    m_traverse.detach(this);
    
    // Pass the call to the base method in scene::Node
    //Node::release(); // greebo: no double deletes, please
  }
  
  InstanceCounter m_instanceCounter;
  void instanceAttach(const scene::Path& path)
  {
    if(++m_instanceCounter.m_count == 1)
    {
      m_traverse.instanceAttach(path_find_mapfile(path.begin(), path.end()));
    }
  }
  void instanceDetach(const scene::Path& path)
  {
    if(--m_instanceCounter.m_count == 0)
    {
      m_traverse.instanceDetach(path_find_mapfile(path.begin(), path.end()));
    }
  }

	// scene::Traversable::Observer implementation
  void insertChild(scene::INodePtr child)
  {
    m_instances.insertChild(child);
  }
  void eraseChild(scene::INodePtr child)
  {
    m_instances.eraseChild(child);
  }

  scene::INodePtr clone() const
  {
    return scene::INodePtr(new MapRoot(*this));
  }

  scene::Instance* create(const scene::Path& path, scene::Instance* parent)
  {
    return new Instance(path, parent);
  }
  void forEachInstance(const scene::Instantiable::Visitor& visitor)
  {
    m_instances.forEachInstance(visitor);
  }
  void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance)
  {
    m_instances.insert(observer, path, instance);
    instanceAttach(path);
  }
  scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path)
  {
    instanceDetach(path);
    return m_instances.erase(observer, path);
  }
};

inline scene::INodePtr NewMapRoot(const std::string& name) {
	return scene::INodePtr(new MapRoot(name));
}


#endif
