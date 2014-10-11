#pragma once

#include "SpecifierPanel.h"
#include "TextSpecifierPanel.h"

namespace objectives {

namespace ce {

/**
 * SpecifierPanel subclass for the SPEC_CLASSNAME (name of entityDef) specifier
 * type.
 */
class ClassnameSpecifierPanel :
	public TextSpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_CLASSNAME().getName(),
				SpecifierPanelPtr(new ClassnameSpecifierPanel())
			);
		}
	} _regHelper;
};

} // namespace ce

} // namespace objectives
