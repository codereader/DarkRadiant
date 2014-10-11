#pragma once

#include "SpecifierPanel.h"
#include "TextSpecifierPanel.h"

namespace objectives {

namespace ce {

/**
 * SpecifierPanel subclass for the SPEC_AI_TYPE (guards, civilians(?)) specifier
 * type.
 */
class AITypeSpecifierPanel :
	public TextSpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_AI_TYPE().getName(),
				SpecifierPanelPtr(new AITypeSpecifierPanel())
			);
		}
	} _regHelper;
};

} // namespace ce

} // namespace objectives
