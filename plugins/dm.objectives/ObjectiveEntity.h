#ifndef OBJECTIVEENTITY_H_
#define OBJECTIVEENTITY_H_

#include "Objective.h"
#include "Logic.h"

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
	scene::INodeWeakPtr _entityNode;
	
	// Map of numbered Objective objects
	ObjectiveMap _objectives;

	// Each difficulty level can have its own mission logic
	// The index -1 is reserved for the default logic
	typedef std::map<int, LogicPtr> LogicMap;
	LogicMap _logics;
	
private:

	// Read the mission success/failure logic from the entity
	void readMissionLogic(Entity* ent);

	// Store the mission logic to the entity
	void writeMissionLogic(Entity* ent);

    // Write the Components to the underlying entity
    void writeComponents(
        Entity* entity, const std::string& keyPrefix, const Objective& objective
    );

	// Removes all objective-related spawnargs from the given entity
	void clearEntity(Entity* entity);

public:

	/**
	 * Construct an ObjectiveEntity wrapper around the given Node.
	 */
	ObjectiveEntity(scene::INodePtr node);
	
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

	// Returns the highest used objective index or -1 if none are present
	int getHighestObjIndex() {
		if (_objectives.empty()) return -1;

		return _objectives.rbegin()->first;
	}

	// Returns the highest used objective index or -1 if none are present
	int getLowestObjIndex() {
		if (_objectives.empty()) return -1;

		return _objectives.begin()->first;
	}

	/**
	 * Add a new objective, starting from the first unused objective ID.
	 */
	void addObjective();

	/**
	 * greebo: Moves the specified objective by the given amount (e.g. +1/-1).
	 */
	void moveObjective(int index, int delta);
	
	/**
	 * Delete a numbered objective. This re-orders all objectives so that the
	 * numbering is consistent again (deleting obj 2 will re-number 3 => 2, 4 => 3, etc.)
	 */
	void deleteObjective(int index);
	
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
	 * greebo: Returns the mission logic structure for the given difficulty level.
	 * The level -1 refers to the default logic structure.
	 *
	 * @returns: The logic (is never NULL). The logic object will be created if 
	 * it isn't already existing, but the logic structure would be empty in that case.
	 */
	LogicPtr getMissionLogic(int difficultyLevel);

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
	void writeToEntity();
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
