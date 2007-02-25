#ifndef OBJECTIVEENTITYFINDER_H_
#define OBJECTIVEENTITYFINDER_H_

#include "iscenegraph.h"
#include "ientity.h"

#include <string>

namespace ui
{

/**
 * Visitor class to locate and list any target_addobjectives entities in the
 * current map.
 */
class ObjectiveEntityFinder
: public scene::Traversable::Walker
{
	// Name of entity class we are looking for
	std::string _className;
	
	// GtkListStore to populate with results
	GtkListStore* _store;
	
public:

	/**
	 *  Construct a visitor to populate the given store.
	 */
	ObjectiveEntityFinder(GtkListStore* st, const std::string& classname)
	: _className(classname),
	  _store(st)
	{ }
	
	/**
	 * Required pre-descent function.
	 */
	bool pre(scene::Node& node) const {
		
		// Get the entity and check the classname
		Entity* e = Node_getEntity(node);
		if (e != NULL 
			&& e->getKeyValue("classname") == _className) 
		{
			// Construct the display string
			std::string sDisplay = 
				"<b>" + e->getKeyValue("name") + "</b> at [ " 
				+ e->getKeyValue("origin") + " ]";
			
			// Add the entity to the list
			GtkTreeIter iter;
			gtk_list_store_append(_store, &iter);
			gtk_list_store_set(_store, &iter, 
							   0, sDisplay.c_str(),
							   1, FALSE,
							   -1); 
		}
		
		return false;
	}

};

}

#endif /*OBJECTIVEENTITYFINDER_H_*/
