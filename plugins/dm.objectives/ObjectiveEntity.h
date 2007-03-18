#ifndef OBJECTIVEENTITY_H_
#define OBJECTIVEENTITY_H_

#include "Objective.h"

#include "ientity.h"
#include "iscenegraph.h"

#include <boost/shared_ptr.hpp>

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
	ObjectiveEntity(scene::Node& n)
	: _node(n),
	  _entity(Node_getEntity(n))
	{ }
	
	/**
	 * Delete the actual entity node from the map. This will render any further
	 * operations on this ObjectiveEntity undefined, and it should immediately
	 * be deleted.
	 */
	void deleteWorldNode() {
		Node_getTraversable(GlobalSceneGraph().root())->erase(_node);
		_entity = NULL;		
	}
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
