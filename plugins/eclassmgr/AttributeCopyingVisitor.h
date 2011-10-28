#pragma once

#include "ieclass.h"

namespace eclass
{

/**
 * Visitor class which copies all of the class attributes from one entity onto
 * another. Used for inheritance resolution, this also sets the "inherited" flag
 * on the EntityClassAttribute.
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
	{}

	virtual ~AttributeCopyingVisitor() {}

	// Required visit function
	void visit(const EntityClassAttribute& attr)
	{
		// greebo: Add the attribute with "inherited" set to true
		_target.addAttribute(EntityClassAttribute(attr, true));
	}
};

}
