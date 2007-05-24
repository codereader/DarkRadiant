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

#include "scenegraph.h"

#include "debugging/debugging.h"

#include <map>
#include <list>
#include <set>
#include <vector>

#include "string/string.h"
#include "signal/signal.h"
#include "scenelib.h"
#include "instancelib.h"
#include "treemodel.h"

/** greebo: This is the actual implementation of the scene::Graph
 * 			defined in iscenegraph.h. This keeps track of all
 * 			the instances.
 */
class CompiledGraph : 
	public scene::Graph, 
	public scene::Instantiable::Observer
{
  typedef std::map<PathConstReference, scene::Instance*> InstanceMap;
	typedef std::list<scene::Graph::Observer*> ObserverList;

	ObserverList _sceneObservers;
  InstanceMap m_instances;
  
	// This is the associated graph tree model (used for the EntityList)
	GraphTreeModel* _treeModel;
	
  Signal0 m_boundsChanged;
  scene::Path m_rootpath;

public:

	CompiledGraph() :
		_treeModel(graph_tree_model_new())
	{}
	
	~CompiledGraph() {
		graph_tree_model_delete(_treeModel);
	}
	
	GraphTreeModel* getTreeModel() {
		return _treeModel;
	}
  
	void addSceneObserver(scene::Graph::Observer* observer) {
		if (observer != NULL) {
			// Add the passed observer to the list
			_sceneObservers.push_back(observer);
		}
	}

	void removeSceneObserver(scene::Graph::Observer* observer) {
		// Cycle through the list of observers and call the moved method
		for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); i++) {
			scene::Graph::Observer* registered = *i;
			
			if (registered == observer) {
				_sceneObservers.erase(i++);
				return; // Don't continue the loop, the iterator is obsolete 
			}
		}
	}

	void sceneChanged() {
		for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); i++) {
			scene::Graph::Observer* observer = *i;
			observer->sceneChanged();
		}
	}

  scene::Node& root()
  {
    ASSERT_MESSAGE(!m_rootpath.empty(), "scenegraph root does not exist");
    return m_rootpath.top();
  }
  void insert_root(scene::Node& root)
  {
    //globalOutputStream() << "insert_root\n";

    ASSERT_MESSAGE(m_rootpath.empty(), "scenegraph root already exists");

    root.IncRef();

    Node_traverseSubgraph(root, InstanceSubgraphWalker(this, scene::Path(), 0));

    m_rootpath.push(makeReference(root));
  }
  void erase_root()
  {
    //globalOutputStream() << "erase_root\n";

    ASSERT_MESSAGE(!m_rootpath.empty(), "scenegraph root does not exist");

    scene::Node& root = m_rootpath.top();

    m_rootpath.pop();

    Node_traverseSubgraph(root, UninstanceSubgraphWalker(this, scene::Path()));

    root.DecRef();
  }
  void boundsChanged()
  {
    m_boundsChanged();
  }

  void traverse(const Walker& walker)
  {
    traverse_subgraph(walker, m_instances.begin());
  }

  void traverse_subgraph(const Walker& walker, const scene::Path& start)
  {
    if(!m_instances.empty())
    {
      traverse_subgraph(walker, m_instances.find(PathConstReference(start)));
    }
  }

  scene::Instance* find(const scene::Path& path)
  {
    InstanceMap::iterator i = m_instances.find(PathConstReference(path));
    if(i == m_instances.end())
    {
      return 0;
    }
    return (*i).second;
  }

  void insert(scene::Instance* instance)
  {
    m_instances.insert(InstanceMap::value_type(PathConstReference(instance->path()), instance));

		// Notify the graph tree model about the change
		sceneChanged();
		graph_tree_model_insert(scene_graph_get_tree_model(), *instance);
  }
  void erase(scene::Instance* instance)
  {
  		// Notify the graph tree model about the change
		sceneChanged();
		graph_tree_model_erase(scene_graph_get_tree_model(), *instance);

    m_instances.erase(PathConstReference(instance->path()));
  }

  SignalHandlerId addBoundsChangedCallback(const SignalHandler& boundsChanged)
  {
    return m_boundsChanged.connectLast(boundsChanged);
  }
  void removeBoundsChangedCallback(SignalHandlerId id)
  {
    m_boundsChanged.disconnect(id);
  }

private:

  bool pre(const Walker& walker, const InstanceMap::iterator& i)
  {
    return walker.pre(i->first, *i->second);
  }

  void post(const Walker& walker, const InstanceMap::iterator& i)
  {
    walker.post(i->first, *i->second);
  }

  void traverse_subgraph(const Walker& walker, InstanceMap::iterator i)
  {
    Stack<InstanceMap::iterator> stack;
    if(i != m_instances.end())
    {
      const std::size_t startSize = (*i).first.get().size();
      do
      {
        if(i != m_instances.end()
          && stack.size() < ((*i).first.get().size() - startSize + 1))
        {
          stack.push(i);
          ++i;
          if(!pre(walker, stack.top()))
          {
            // skip subgraph
            while(i != m_instances.end()
              && stack.size() < ((*i).first.get().size() - startSize + 1))
            {
              ++i;
            }
          }
        }
        else
        {
          post(walker, stack.top());
          stack.pop();
        }
      }
      while(!stack.empty());
    }
  }
};

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class SceneGraphAPI
{
	typedef boost::shared_ptr<CompiledGraph> CompiledGraphPtr;
	CompiledGraphPtr _sceneGraph;
public:
  typedef scene::Graph Type;
  STRING_CONSTANT(Name, "*");

	SceneGraphAPI() {
		_sceneGraph = CompiledGraphPtr(new CompiledGraph());
	}

	scene::Graph* getTable() {
		// Return the contained pointer to the CompiledGraph
		return _sceneGraph.get();
	}
};

typedef SingletonModule<SceneGraphAPI> SceneGraphModule;
typedef Static<SceneGraphModule> StaticSceneGraphModule;
StaticRegisterModule staticRegisterSceneGraph(StaticSceneGraphModule::instance());

GraphTreeModel* scene_graph_get_tree_model() {
	CompiledGraph* sceneGraph = static_cast<CompiledGraph*>(
		StaticSceneGraphModule::instance().getTable()
	);
	return sceneGraph->getTreeModel();
}
