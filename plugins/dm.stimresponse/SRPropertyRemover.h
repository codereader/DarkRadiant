#ifndef SRPROPERTYREMOVER_H_
#define SRPROPERTYREMOVER_H_

#include "ientity.h"

#include "SREntity.h"
#include "StimTypes.h"

/** greebo: This visitor removes all the Stim/Response-related
 * 			spawnargs from the visited entity.
 * 
 * 			It first visits all the keyvalues, and adds the 
 * 			matching items to an internal list. Upon destruction,
 * 			the list is traversed and the keys are deleted.
 *			(in-loop deletion will crash the app as the iterators
 * 			 get invalidated by the deletion...)
 * 
 * 			Use this before saving the new values to the entity.
 */
class SRPropertyRemover :
	public Entity::Visitor
{
	// The entity to be cleaned
	Entity* _target;

	// The reference to the list of possible key names
	SREntity::KeyList& _keys;

	typedef std::vector<std::string> KeyList;
	// The list of keys that have to be removed after the walk
	KeyList _removeList;

	// Local helper class containing all the stimtypes
	StimTypes _stimTypes;

public:
	// Constructor
	SRPropertyRemover(Entity* target, SREntity::KeyList& keys);
	
	// Destructor, this is where the values are destroyed.
	~SRPropertyRemover();
	
	// Entity::Visitor implementation
	void visit(const std::string& key, const std::string& value);
};

#endif /*SRPROPERTYREMOVER_H_*/
