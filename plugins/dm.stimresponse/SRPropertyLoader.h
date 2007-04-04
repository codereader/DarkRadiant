#ifndef SRPROPERTYLOADER_H_
#define SRPROPERTYLOADER_H_

#include "ieclass.h"
#include "ientity.h"
#include "SREntity.h"
#include "StimTypes.h"

class SRPropertyLoader :
	public Entity::Visitor,
	public EntityClassAttributeVisitor
{
	// The reference to the list of possible key names
	SREntity::KeyList& _keys;
	
	// The target list where all the StimResponse objects will be stored
	SREntity::StimResponseMap& _srMap;

	// The target string for storing the parse warnings
	std::string& _warnings;
	
	// Local helper class containing all the stimtypes
	StimTypes _stimTypes;
	
public:
	// Constructor
	SRPropertyLoader(SREntity::KeyList& keys, 
					 SREntity::StimResponseMap& srMap,
					 std::string& warnings);
	
	// Entity::Visitor implementation
	void visit(const std::string& key, const std::string& value);
	
	// EntityClassAttributeVisitor implementation
	void visit(const EntityClassAttribute& attribute);
	
private:
	
	/** greebo: Private helper method that does the attribute analysis
	 * 			and adds the SR items to the target list
	 * 
	 * @inherited: reflects whether this key/value stems from a inheritance  
	 */
	void parseAttribute(const std::string& key, 
						const std::string& value, 
						bool inherited);
	
};

#endif /*SRPROPERTYLOADER_H_*/
