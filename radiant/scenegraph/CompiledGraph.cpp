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

#include "CompiledGraph.h"

#include "debugging/debugging.h"
#include "scenelib.h"
#include "instancelib.h"
#include "treemodel.h"
#include "SceneGraphModule.h"

CompiledGraph::CompiledGraph() :
	_treeModel(graph_tree_model_new())
{}

CompiledGraph::~CompiledGraph() {
	graph_tree_model_delete(_treeModel);
}

GraphTreeModel* CompiledGraph::getTreeModel() {
	return _treeModel;
}
  
void CompiledGraph::addSceneObserver(scene::Graph::Observer* observer) {
	if (observer != NULL) {
		// Add the passed observer to the list
		_sceneObservers.push_back(observer);
	}
}

void CompiledGraph::removeSceneObserver(scene::Graph::Observer* observer) {
	// Cycle through the list of observers and call the moved method
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); i++) {
		scene::Graph::Observer* registered = *i;
		
		if (registered == observer) {
			_sceneObservers.erase(i++);
			return; // Don't continue the loop, the iterator is obsolete 
		}
	}
}

void CompiledGraph::sceneChanged() {
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); i++) {
		scene::Graph::Observer* observer = *i;
		observer->sceneChanged();
	}
}

scene::INodePtr CompiledGraph::root() {
	ASSERT_MESSAGE(!m_rootpath.empty(), "scenegraph root does not exist");
	return m_rootpath.top();
}
  
void CompiledGraph::insert_root(scene::INodePtr root) {
	ASSERT_MESSAGE(m_rootpath.empty(), "scenegraph root already exists");

    Node_traverseSubgraph(root, InstanceSubgraphWalker(scene::Path(), 0));

    m_rootpath.push(root);
}
  
void CompiledGraph::erase_root() {
    ASSERT_MESSAGE(!m_rootpath.empty(), "scenegraph root does not exist");

    scene::INodePtr root = m_rootpath.top();

    m_rootpath.pop();

    Node_traverseSubgraph(root, UninstanceSubgraphWalker(scene::Path()));
}

void CompiledGraph::boundsChanged() {
    m_boundsChanged();
}

void CompiledGraph::traverse(const Walker& walker) {
    traverse_subgraph(walker, m_instances.begin());
}

void CompiledGraph::traverse_subgraph(const Walker& walker, const scene::Path& start) {
    if(!m_instances.empty()) {
      traverse_subgraph(walker, m_instances.find(PathConstReference(start)));
    }
}

scene::Instance* CompiledGraph::find(const scene::Path& path) {
    InstanceMap::iterator i = m_instances.find(PathConstReference(path));
    if(i == m_instances.end()) {
      return 0;
    }
    return (*i).second;
}

void CompiledGraph::insert(scene::Instance* instance) {
    m_instances.insert(InstanceMap::value_type(PathConstReference(instance->path()), instance));

	// Notify the graph tree model about the change
	sceneChanged();
	graph_tree_model_insert(scene_graph_get_tree_model(), *instance);
}

void CompiledGraph::erase(scene::Instance* instance) {
  	// Notify the graph tree model about the change
	sceneChanged();
	graph_tree_model_erase(scene_graph_get_tree_model(), *instance);

    m_instances.erase(PathConstReference(instance->path()));
}

SignalHandlerId CompiledGraph::addBoundsChangedCallback(const SignalHandler& boundsChanged) {
    return m_boundsChanged.connectLast(boundsChanged);
}

void CompiledGraph::removeBoundsChangedCallback(SignalHandlerId id) {
    m_boundsChanged.disconnect(id);
}

bool CompiledGraph::pre(const Walker& walker, const InstanceMap::iterator& i) {
    return walker.pre(i->first, *i->second);
}

void CompiledGraph::post(const Walker& walker, const InstanceMap::iterator& i) {
    walker.post(i->first, *i->second);
}

void CompiledGraph::traverse_subgraph(const Walker& walker, InstanceMap::iterator i) {
	std::stack<InstanceMap::iterator> stack;
	if (i != m_instances.end()) {
		// Initialise the start size using the path depth of the given iterator
		const std::size_t startSize = i->first.get().size();
		
		do {
			if (i != m_instances.end() && 
				stack.size() < (i->first.get().size() - startSize + 1))
			{
				stack.push(i);
				++i;
				if (!pre(walker, stack.top())) {
					// Walker's pre() return false, skip subgraph
					while (i != m_instances.end() && 
						   stack.size() < (i->first.get().size() - startSize + 1))
					{
						++i;
					}
				}
			}
			else {
				post(walker, stack.top());
				stack.pop();
			}
		} while (!stack.empty());
	}
}
