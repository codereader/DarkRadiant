#ifndef ENTITYKEYVALUEVISITOR_H_
#define ENTITYKEYVALUEVISITOR_H_

#include "ientity.h"
#include <map>
#include <string>

// Typedefs

typedef std::map<const std::string, const std::string> KeyValueMap;

/* EntityKeyValueVisitor
 * 
 * This class acts as a Visitor for an Entity object. It builds a list of keys
 * and values attached to the Entity, and adds them into a hashtable.
 */

class EntityKeyValueVisitor:
    public Entity::Visitor
{
    // Map to contain keys->values
    KeyValueMap _keyValueMap;
    
public:

	// Constructor
	EntityKeyValueVisitor();
    
    // Main visit function
    virtual void visit(const char* key, const char* value);
    
    // Return the map object
    KeyValueMap& getMap() {
        return _keyValueMap;
    }
};

#endif /*ENTITYKEYVALUEVISITOR_H_*/
