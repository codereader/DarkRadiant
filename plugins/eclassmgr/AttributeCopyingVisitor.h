#ifndef ATTRIBUTECOPYINGVISITOR_H_
#define ATTRIBUTECOPYINGVISITOR_H_

#include "ieclass.h"

namespace eclass
{

/**
 * Visitor class which copies all of the class attributes from one entity onto
 * another. Used for inheritance resolution.
 */
class AttributeCopyingVisitor
: public EntityClassAttributeVisitor
{
	// Target entity to copy values onto
	IEntityClass& _target;
	
public:

	// Constructor sets target
	AttributeCopyingVisitor(IEntityClass& target)
	: _target(target)
	{ }

	// Required visit function
	void visit(const EntityClassAttribute& attr) {
		_target.addAttribute(attr);
	}
};

}

#endif /*ATTRIBUTECOPYINGVISITOR_H_*/
