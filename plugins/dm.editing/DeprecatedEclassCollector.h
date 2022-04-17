#pragma once

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
		const std::string attr = eclass->getAttributeValue("editor_replacement");
		if (attr.empty())
			return;

		// Non-empty editor_replacement, add fixup code
		_fixupCode += ENTITYDEF_PREFIX + eclass->getName() + " => " + attr + "\n";
	}

	const std::string& getFixupCode() const
	{
		return _fixupCode;
	}
};
