#ifndef SPECIFIERPANELFACTORY_H_
#define SPECIFIERPANELFACTORY_H_

#include "SpecifierPanel.h"
#include "../../SpecifierType.h"

#include <map>

namespace objectives
{

namespace ce
{

/**
 * Factory class for creating SpecifierPanel subclasses for a particular type
 * of SpecifierType.
 */
class SpecifierPanelFactory
{
	// Static map instance
	typedef std::map<std::string, SpecifierPanelPtr> PanelMap;
	static PanelMap& getMap();

public:

	/**
	 * Register a SpecifierPanel subclass.
	 *
	 * This method is invoked by SpecifierPanel subclasses to register
	 * themselves for virtual construction. Once a SpecifierPanel is registered
	 * it can be created and returned by the SpecifierPanelFactory::create()
	 * method based on its SpecifierType type.
	 *
	 * @param name
	 * Name of the SpecifierType type that this panel will edit.
	 *
	 * @param cls
	 * The SpecifierPanel subclass to register.
	 */
	static void registerType(const std::string& name,
							 SpecifierPanelPtr cls);

	/**
	 * Create a SpecifierPanel to edit the given SpecifierType type.
	 *
	 * @param name
	 * Name of the SpecifierType type for which a SpecifierPanel must be created.
	 *
	 * @return
	 * Shared pointer to a SpecifierPanel which contains widgets to edit this
	 * SpecifierType type. If no SpecifierPanl has been registered for this type,
	 * a NULL shared_ptr is returned.
	 */
	static SpecifierPanelPtr create(const std::string& name);
};

}

}

#endif /*SPECIFIERPANELFACTORY_H_*/
