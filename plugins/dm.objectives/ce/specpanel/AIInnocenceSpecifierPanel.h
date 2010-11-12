#ifndef AI_INNOCENCE_SPECIFIERPANEL_H_
#define AI_INNOCENCE_SPECIFIERPANEL_H_

#include "SpecifierPanel.h"
#include "TextSpecifierPanel.h"

namespace objectives {

namespace ce {

/**
 * SpecifierPanel subclass for the SPEC_AI_INNOCENCE specifier type.
 */
class AIInnocenceSpecifierPanel :
	public TextSpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_AI_INNOCENCE().getName(),
				SpecifierPanelPtr(new AIInnocenceSpecifierPanel())
			);
		}
	} _regHelper;
};

} // namespace ce

} // namespace objectives

#endif /* AI_INNOCENCE_SPECIFIERPANEL_H_ */
