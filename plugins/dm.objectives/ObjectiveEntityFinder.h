#ifndef OBJECTIVEENTITYFINDER_H_
#define OBJECTIVEENTITYFINDER_H_

#include "iscenegraph.h"
#include "ientity.h"

#include <string>

namespace objectives
{

/**
 * Visitor class to locate and list any target_addobjectives entities in the
 * current map. Also keeps a reference to the worldspawn entity so that the
 * "activate at start" status can be determined (the worldspawn targets any
 * objective entities that should be active at start).
 */
class ObjectiveEntityFinder
: public scene::Traversable::Walker
{
	// Name of entity class we are looking for
	std::string _className;
	
	// GtkListStore to populate with results
	GtkListStore* _store;
	
	// Worldspawn entity
	mutable Entity* _worldSpawn;
	
public:

	/**
	 *  Construct a visitor to populate the given store.
	 */
	ObjectiveEntityFinder(GtkListStore* st, const std::string& classname)
	: _className(classname),
	  _store(st)
	{ }
	
	/**
	 * Return a pointer to the worldspawn entity.
	 */
	Entity* getWorldSpawn() {
		return _worldSpawn;
	}
	
	/**
	 * Required pre-descent function.
	 */
	bool pre(scene::Node& node) const {
		
		// Get the entity and check the classname
		Entity* ePtr = Node_getEntity(node);
		if (ePtr != NULL 
			&& ePtr->getKeyValue("classname") == _className) 
		{
			// Construct the display string
			std::string sDisplay = 
				"<b>" + ePtr->getKeyValue("name") + "</b> at [ " 
				+ ePtr->getKeyValue("origin") + " ]";
			
			// Add the entity to the list
			GtkTreeIter iter;
			gtk_list_store_append(_store, &iter);
			gtk_list_store_set(_store, &iter, 
							   0, sDisplay.c_str(),
							   1, FALSE,	// active at start
							   2, ePtr,		// pointer to Entity
							   3, &node,	// pointer to raw Node
							   -1); 
		}
		else if (ePtr != NULL 
				 && ePtr->getKeyValue("classname") == "worldspawn")
		{
			_worldSpawn = ePtr;	
		}
		
		return false;
	}

};

}

#endif /*OBJECTIVEENTITYFINDER_H_*/
