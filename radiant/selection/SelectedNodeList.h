#ifndef SELECTEDNODELIST_H_
#define SELECTEDNODELIST_H_

#include <map>
#include "scenelib.h"

/**
 * greebo: This container adapts a std::multimap to keep track of 
 *         all the selected nodes in the scene. Additionally,
 *         the insertion order is remembered to allow for retrieval
 *         of the ultimate/penultimate selected node.
 * 
 * It also allows for the same node occuring multiple times in
 * the map at once. On deletion, the node which has been added
 * latest is removed.
 * 
 * It is not as fast when inserting new elements as std::list but
 * allows for fast lookup of arbitrary nodes.
 * 
 * Also, the map interface is extended by the penultimate() accessor
 * which is needed by the RadiantSelectionSystem. 
 */
class SelectedNodeList :
	public std::multimap<scene::INodePtr, unsigned long>
{
	// Shortcut typedef
	typedef std::multimap<scene::INodePtr, unsigned long> MapType;
	
	// This is an ever-incrementing counter, some sort of "insertion time"
	static unsigned long time; 
public:
	/**
	 * greebo: Returns the element which has been inserted last.
	 * 
	 * Note: Runs in linear time = SLOW
	 */
	const scene::INodePtr& ultimate();
	const scene::INodePtr& ultimate() const;
	
	/**
	 * greebo: Returns the element right before the last selected.
	 * 
	 * Note: Runs in linear time = SLOW
	 */
	const scene::INodePtr& penultimate() const;
	const scene::INodePtr& penultimate();
	
	/**
	 * greebo: Inserts a new element to this container.
	 *         Multiple insertions of the same elements are 
	 *         allowed, but their insertion "time" is of course different.
	 */
	void append(const scene::INodePtr& selected);
	
	/**
	 * greebo: Removes the node which has been selected last
	 *         from this map. If multiple nodes with the same
	 *         address exist in the map, only the one with the 
	 *         highest time is removed, the others are left. 
	 */
	void erase(const scene::INodePtr& selected);
};

#endif /*SELECTEDNODELIST_H_*/
