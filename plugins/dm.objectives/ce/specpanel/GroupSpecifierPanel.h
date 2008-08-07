#ifndef GROUPSPECIFIERPANEL_H_
#define GROUPSPECIFIERPANEL_H_

#include "SpecifierPanel.h"
#include "TextSpecifierPanel.h"

namespace objectives {

namespace ce {

/**
 * SpecifierPanel subclass for the SPEC_GROUP specifier type.
 */
class GroupSpecifierPanel : 
	public TextSpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() { 
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_GROUP().getName(),
				SpecifierPanelPtr(new GroupSpecifierPanel())
			);
		}
	} _regHelper;
};

} // namespace objectives

} // namespace ce

#endif /* GROUPSPECIFIERPANEL_H_ */
