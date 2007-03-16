#ifndef SRPROPERTYLOADER_H_
#define SRPROPERTYLOADER_H_

#include "ieclass.h"
#include "ientity.h"
#include "SREntity.h"

class SRPropertyLoader :
	public Entity::Visitor,
	public EntityClassAttributeVisitor
{

	// The reference to the list of possible key names
	SREntity::KeyList& _keys;
	
	// The target list where all the StimResponse objects will be stored
	SREntity::SRList& _srList;

public:
	// Constructor
	SRPropertyLoader(SREntity::KeyList& keys, SREntity::SRList& srList);
	
	// Entity::Visitor implementation
	void visit(const std::string& key, const std::string& value);
	
	// EntityClassAttributeVisitor implementation
	void visit(const EntityClassAttribute& attribute);
	
private:
	
	/** greebo: Private helper method that does the attribute analysis
	 * 			and adds the SR items to the target list
	 */
	void parseAttribute(const std::string& key, const std::string& value);
	
};

#endif /*SRPROPERTYLOADER_H_*/
