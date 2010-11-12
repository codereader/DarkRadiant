#ifndef SPAWNCLASS_SPECIFIER_PANEL_H_
#define SPAWNCLASS_SPECIFIER_PANEL_H_

#include "SpecifierPanel.h"
#include "TextSpecifierPanel.h"

namespace objectives {

namespace ce {

/**
 * SpecifierPanel subclass for the SPEC_SPAWNCLASS (name of C++ class) specifier
 * type.
 */
class SpawnClassSpecifierPanel :
	public TextSpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_SPAWNCLASS().getName(),
				SpecifierPanelPtr(new SpawnClassSpecifierPanel())
			);
		}
	} _regHelper;
};

} // namespace ce

} // namespace objectives

#endif /* SPAWNCLASS_SPECIFIER_PANEL_H_ */
