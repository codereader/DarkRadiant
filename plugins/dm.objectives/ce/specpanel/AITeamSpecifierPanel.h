#pragma once

#include "SpecifierPanel.h"
#include "TextSpecifierPanel.h"

namespace objectives {

namespace ce {

/**
 * SpecifierPanel subclass for the SPEC_AI_TEAM (number) specifier
 * type.
 */
class AITeamSpecifierPanel :
	public TextSpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_AI_TEAM().getName(),
				SpecifierPanelPtr(new AITeamSpecifierPanel())
			);
		}
	} _regHelper;
};

} // namespace ce

} // namespace objectives
