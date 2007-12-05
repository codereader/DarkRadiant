#ifndef SELECTEDINSTANCELIST_H_
#define SELECTEDINSTANCELIST_H_

#include <map>
#include "scenelib.h"

/**
 * greebo: This container adapts a std::multimap to keep track of 
 *         all the selected instances in the scene. Additionally,
 *         the insertion order is remembered to allow for retrieval
 *         of the ultimate/penultimate selected instance.
 * 
 * It also allows for the same Instance occuring multiple times in
 * the map at once. On deletion, the Instance which has been added
 * latest is removed.
 * 
 * It is not as fast when inserting new elements as std::list but
 * allows for fast lookup of arbitrary instances.
 * 
 * Also, the map interface is extended by the penultimate() accessor
 * which is needed by the RadiantSelectionSystem. 
 */
class SelectedInstanceList :
	public std::multimap<scene::Instance*, unsigned long>
{
	// Shortcut typedef
	typedef std::multimap<scene::Instance*, unsigned long> MapType;
	
	// This is an ever-incrementing counter, some sort of "insertion time"
	static unsigned long time; 
public:
	/**
	 * greebo: Returns the element which has been inserted last.
	 * 
	 * Note: Runs in linear time = SLOW
	 */
	scene::Instance& ultimate();
	scene::Instance& ultimate() const;
	
	/**
	 * greebo: Returns the element right before the last selected.
	 * 
	 * Note: Runs in linear time = SLOW
	 */
	scene::Instance& penultimate() const;
	scene::Instance& penultimate();
	
	/**
	 * greebo: Inserts a new element to this container.
	 *         Multiple insertions of the same elements are 
	 *         allowed, but their insertion "time" is of course different.
	 */
	void append(scene::Instance& selected);
	
	/**
	 * greebo: Removes the instance which has been selected last
	 *         from this map. If multiple instances with the same
	 *         address exist in the map, only the one with the 
	 *         highest time is removed, the others are left. 
	 */
	void erase(scene::Instance& selected);
};

#endif /*SELECTEDINSTANCELIST_H_*/
