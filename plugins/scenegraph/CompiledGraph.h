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

#if !defined(INCLUDED_SCENEGRAPH_H)
#define INCLUDED_SCENEGRAPH_H

#include <map>
#include <list>
#include "signal/signal.h"
#include "scenelib.h"
#include "imodule.h"

/** greebo: This is the actual implementation of the scene::Graph
 * 			defined in iscenegraph.h. This keeps track of all
 * 			the instances.
 */
class CompiledGraph : 
	public scene::Graph
{
	typedef std::map<PathConstReference, scene::Instance*> InstanceMap;
	InstanceMap m_instances;

	typedef std::list<scene::Graph::Observer*> ObserverList;
	ObserverList _sceneObservers;

	Signal0 m_boundsChanged;
	scene::Path m_rootpath;

public:	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
  
	/** greebo: Adds/removes an observer from the scenegraph,
	 * 			to get notified upon insertions/deletions
	 */
	void addSceneObserver(scene::Graph::Observer* observer);
	void removeSceneObserver(scene::Graph::Observer* observer);

	// Triggers a call to all the connected Scene::Graph::Observers
	void sceneChanged();

	// Root node accessor methods
	scene::INodePtr root();
	void insert_root(scene::INodePtr root);
	void erase_root();

	// greebo: Emits the "bounds changed" signal to all connected observers
	// Note: these are the WorkZone and the SelectionSystem, AFAIK
	void boundsChanged();

	void traverse(const Walker& walker);
	void traverse_subgraph(const Walker& walker, const scene::Path& start);

	scene::Instance* find(const scene::Path& path);

	void insert(scene::Instance* instance);
	void erase(scene::Instance* instance);

	SignalHandlerId addBoundsChangedCallback(const SignalHandler& boundsChanged);
	void removeBoundsChangedCallback(SignalHandlerId id);

private:
	bool pre(const Walker& walker, const InstanceMap::iterator& i);
	void post(const Walker& walker, const InstanceMap::iterator& i);

	void traverse_subgraph(const Walker& walker, InstanceMap::iterator i);
};
typedef boost::shared_ptr<CompiledGraph> CompiledGraphPtr;

#endif
