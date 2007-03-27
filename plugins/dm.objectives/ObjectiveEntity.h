#ifndef OBJECTIVEENTITY_H_
#define OBJECTIVEENTITY_H_

#include "Objective.h"

#include "ientity.h"
#include "iscenegraph.h"

#include <boost/shared_ptr.hpp>
#include <gtk/gtkliststore.h>

// FORWARD DECLS
namespace objectives { class TargetList; }

namespace objectives
{

/**
 * Representation of a single objective entity (target_tdm_addobjectives). This
 * is a wrapper which contains an Entity*, and provides additional methods to
 * accept and query Objectives, and to save and load the set of Objectives
 * from the entity's key values.
 */
class ObjectiveEntity
{
	// The actual entity's world node and entity pointer
	scene::Node& _node;
	Entity* _entity;
	
	// Map of numbered Objective objects
	ObjectiveMap _objectives;
	
public:

	/**
	 * Construct an ObjectiveEntity wrapper around the given Node.
	 */
	ObjectiveEntity(scene::Node& n);
	
	/**
	 * Return an Objective reference by numeric index.
	 */
	Objective& getObjective(int iIndex) {
		return _objectives[iIndex];
	}
	
	/**
	 * Delete the actual entity node from the map. This will render any further
	 * operations on this ObjectiveEntity undefined, and it should immediately
	 * be deleted.
	 */
	void deleteWorldNode() {
		Node_getTraversable(GlobalSceneGraph().root())->erase(_node);
		_entity = NULL;		
	}
	
	/**
	 * Test whether this Objective Entity is on the provided TargetList, to
	 * determine whether the entity is a target of another entity (e.g. the
	 * worldspawn).
	 */
	bool isOnTargetList(const TargetList& list) const;
	
	/**
	 * Populate the given list store with the objectives from this entity.
	 * 
	 * @param store
	 * The list store to populate. This must have 2 columns -- an integer 
	 * column for the objective number, and a text column for the description.
	 */
	void populateListStore(GtkListStore* store) const;
};

/**
 * Objective entity pointer type.
 */
typedef boost::shared_ptr<ObjectiveEntity> ObjectiveEntityPtr;

/**
 * Objective entity named map type.
 */
typedef std::map<std::string, ObjectiveEntityPtr> ObjectiveEntityMap;


}

#endif /*OBJECTIVEENTITY_H_*/
