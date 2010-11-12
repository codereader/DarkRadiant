#ifndef ENTITYNAMESPECIFIERPANEL_H_
#define ENTITYNAMESPECIFIERPANEL_H_

#include "SpecifierPanel.h"
#include "TextSpecifierPanel.h"

namespace objectives
{

namespace ce
{

/**
 * SpecifierPanel subclass for the SPEC_NAME (name of single entity) specifier
 * type.
 */
class EntityNameSpecifierPanel
: public TextSpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_NAME().getName(),
				SpecifierPanelPtr(new EntityNameSpecifierPanel())
			);
		}
	} _regHelper;
};


}

}
#endif /* ENTITYNAMESPECIFIERPANEL_H_ */
