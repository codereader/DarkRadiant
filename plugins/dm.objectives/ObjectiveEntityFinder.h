#ifndef OBJECTIVEENTITYFINDER_H_
#define OBJECTIVEENTITYFINDER_H_

#include "scenelib.h"

#include <string>

namespace objectives
{

/**
 * Visitor class to locate and list any <b>target_addobjectives</b> entities in 
 * the current map.
 *
 * The ObjectiveEntityFinder will visit each scenegraph node in turn, as per the
 * behaviour of a scenegraph walker. The classname of each entity visited is
 * tested against a given value (passed in during construction) which identifies
 * it as an Objectives entity, and if the test is successful, the entity's
 * details are added to the target ObjectiveEntityMap and GtkListStore objects
 * to be populated. 
 * 
 * The ObjectiveEntityFinder also keeps a reference to the worldspawn entity so 
 * that the "activate at start" status can be determined (the worldspawn targets 
 * any objective entities that should be active at start).
 */
class ObjectiveEntityFinder
: public scene::Graph::Walker
{
	// Name of entity class we are looking for
	std::string _className;
	
	// GtkListStore to populate with results
	GtkListStore* _store;
	
	// ObjectiveEntityMap which we also populate
	ObjectiveEntityMap& _map;
	
	// Worldspawn entity
	mutable Entity* _worldSpawn;
	
public:

	/**
	 * Construct a visitor to populate the given store and ObjectiveEntityMap.
	 * 
	 * The GtkListStore provided must contain three columns. The first column
	 * is a G_TYPE_STRING containing the display name of the Objectives entity, 
	 * which is constructed from the real entity name plus the origin in 
	 * brackets for convenience purposes. The second column is a G_TYPE_BOOL 
	 * which is set to TRUE if the entity is activated at start, and FALSE 
	 * otherwise. The third column is a G_TYPE_STRING containing the raw entity
	 * name in the map.
	 * 
	 * @param st
	 * The GtkListStore to populate.
	 * 
	 * @param map
	 * The ObjectiveEntityMap to populate.
	 * 
	 * @param classname
	 * The text classname used to identify an Objectives entity.
	 */
	ObjectiveEntityFinder(GtkListStore* st, 
						  ObjectiveEntityMap& map,
						  const std::string& classname)
	: _className(classname),
	  _store(st),
	  _map(map),
	  _worldSpawn(NULL)
	{ }
	
	/**
	 * Return a pointer to the worldspawn entity. This could potentially be
	 * NULL if a worldspawn entity was not found during visitation.
	 */
	Entity* getWorldSpawn() {
		return _worldSpawn;
	}
	
	/**
	 * @see scene::Graph::Walker::pre()
	 */
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		
		// Get the entity and check the classname
		Entity* ePtr = Node_getEntity(path.top());
		if (!ePtr)
			return true;
			
		// Check for objective entity or worldspawn	
		if (ePtr->getKeyValue("classname") == _className) 
		{
			// Construct the display string
			std::string name = ePtr->getKeyValue("name");
			std::string sDisplay = "<b>" + name + "</b> at [ "	
								   + ePtr->getKeyValue("origin") + " ]";
			
			// Add the entity to the list
			GtkTreeIter iter;
			gtk_list_store_append(_store, &iter);
			gtk_list_store_set(_store, &iter, 
							   0, sDisplay.c_str(),
							   1, FALSE,				// active at start
							   2, name.c_str(), 		// raw name
							   -1);
							   
			// Construct an ObjectiveEntity with the node, and add to the map
			ObjectiveEntityPtr oe(new ObjectiveEntity(path.top()));
			_map.insert(ObjectiveEntityMap::value_type(name, oe));
		}
		else if (ePtr->getKeyValue("classname") == "worldspawn")
		{
			_worldSpawn = ePtr;	
		}
		
		return true;
	}

};

}

#endif /*OBJECTIVEENTITYFINDER_H_*/
