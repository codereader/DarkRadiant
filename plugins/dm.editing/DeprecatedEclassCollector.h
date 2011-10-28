#ifndef _DEPRECATED_ECLASS_COLLECTOR_H_
#define _DEPRECATED_ECLASS_COLLECTOR_H_

#include "ieclass.h"

namespace
{
	const std::string MATERIAL_PREFIX("MATERIAL: ");
	const std::string ENTITYDEF_PREFIX("ENTITYDEF: ");
}

class DeprecatedEclassCollector :
	public EntityClassVisitor
{
private:
	std::string _fixupCode;

public:
	void visit(const IEntityClassPtr& eclass)
	{
		const EntityClassAttribute& attr = eclass->getAttribute("editor_replacement");

		if (attr.getValue().empty())
		{
			return;
		}

		// Non-empty editor_replacement, add fixup code
		_fixupCode += ENTITYDEF_PREFIX + eclass->getName() + " => " + attr.getValue() + "\n";
	}

	const std::string& getFixupCode() const
	{
		return _fixupCode;
	}
};

#endif /* _DEPRECATED_ECLASS_COLLECTOR_H_ */
