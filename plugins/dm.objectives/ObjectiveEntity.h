#ifndef OBJECTIVEENTITY_H_
#define OBJECTIVEENTITY_H_

#include "Objective.h"

#include "inode.h"
#include <boost/shared_ptr.hpp>
#include <gtk/gtkliststore.h>

// FORWARD DECLS
namespace objectives { class TargetList; }
class Entity;

namespace objectives
{

/**
 * Representation of a single objective entity (target_tdm_addobjectives). 
 * 
 * In the Dark Mod, objectives are stored as numbered spawnargs on an objective
 * entity, e.g. <b>obj3_desc</b>. Each objective entity can contain any number
 * of objectives described in this way.
 * 
 * The ObjectiveEntity class provides an object-oriented view of the objective
 * information, by wrapping a pointer to an Entity and providing methods to
 * retrieve and manipulate objective information without seeing the spawnargs
 * directly. When changes are completed, the ObjectiveEntity::writeToEntity()
 * method is invoked to save all changes in the form of spawnargs.
 * 
 * @see Entity
 * @see objectives::Objective
 */
class ObjectiveEntity
{
	// The actual entity's world node and entity pointer
	scene::INodePtr _node;
	Entity* _entity;
	
	// Map of numbered Objective objects
	ObjectiveMap _objectives;
	
public:

	/**
	 * Construct an ObjectiveEntity wrapper around the given Node.
	 */
	ObjectiveEntity(scene::INodePtr n);
	
	/**
	 * Return an Objective reference by numeric index.
	 * 
	 * @param iIndex
	 * The numberic index of the objective to retrieve.
	 * 
	 * @return
	 * A non-const reference to an Objective object, corresponding to the 
	 * given index. If the provided index did not previously exist, a new
	 * Objective object will be created and returned.
	 */
	Objective& getObjective(int iIndex) {
		return _objectives[iIndex];
	}

	/**
	 * Add a new objective, starting from the first unused objective ID.
	 */
	void addObjective();
	
	/**
	 * Delete a numbered objective.
	 */
	void deleteObjective(int index) {
		_objectives.erase(index);
	}
	
	/**
	 * Clear all objectives.
	 */
	void clearObjectives() {
		_objectives.clear();	
	}
	
	/**
	 * Test whether this entity contains objectives or not.
	 */
	bool isEmpty() const {
		return _objectives.empty();
	}
	
	/**
	 * Delete the actual entity node from the map. This will render any further
	 * operations on this ObjectiveEntity undefined, and it should immediately
	 * be deleted.
	 */
	void deleteWorldNode();
	
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
	
	/**
	 * Write all objective data to keyvals on the underlying entity.
	 */
	void writeToEntity() const;
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
