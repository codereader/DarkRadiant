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
#include "ispacepartition.h"

namespace scene
{

/** 
 * Implementing class for the scenegraph.
 * 
 * \ingroup scenegraph
 * \see scene::Graph
 */
class SceneGraph : 
	public Graph
{
	typedef std::list<Graph::Observer*> ObserverList;
	ObserverList _sceneObservers;

	Signal0 m_boundsChanged;
	
	// The root-element, the scenegraph starts here
	scene::INodePtr _root;

	// The space partitioning system
	ISpacePartitionSystemPtr _spacePartition;

	std::size_t _visitedSPNodes;
	std::size_t _skippedSPNodes;

public:	
	SceneGraph();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
  
	/** greebo: Adds/removes an observer from the scenegraph,
	 * 			to get notified upon insertions/deletions
	 */
	void addSceneObserver(Graph::Observer* observer);
	void removeSceneObserver(Graph::Observer* observer);

	// Triggers a call to all the connected Scene::Graph::Observers
	void sceneChanged();

	// Root node accessor methods
	const INodePtr& root() const;
	void setRoot(const INodePtr& newRoot);

	// greebo: Emits the "bounds changed" signal to all connected observers
	// Note: these are the WorkZone and the SelectionSystem, AFAIK
	void boundsChanged();

	void insert(const INodePtr& node);
	void erase(const INodePtr& node);

	SignalHandlerId addBoundsChangedCallback(const SignalHandler& boundsChanged);
	void removeBoundsChangedCallback(SignalHandlerId id);

	void nodeBoundsChanged(const scene::INodePtr& node);

	void foreachNodeInVolume(const VolumeTest& volume, Walker& walker);
	void foreachVisibleNodeInVolume(const VolumeTest& volume, Walker& walker);

	ISpacePartitionSystemPtr getSpacePartition();
private:
	// Recursive method used to descend the SpacePartition tree, returns FALSE if the walker signaled stop
	bool foreachNodeInVolume_r(const ISPNode& node, const VolumeTest& volume, Walker& walker, bool visitHidden);
};
typedef boost::shared_ptr<SceneGraph> SceneGraphPtr;

} // namespace scene

#endif
