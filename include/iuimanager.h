#ifndef INCLUDE_UIMANAGER_H_
#define INCLUDE_UIMANAGER_H_

#include <string>
#include "generic/constant.h"

/** greebo: The UI Manager abstract base class.
 * 
 * The UIManager provides an interface to add UI items like menu commands
 * toolbar icons, update status bar texts and such. 
 */
class IUIManager
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "UIManager");

	virtual void addMenuItem(const std::string& menuPath, const std::string& commandName) = 0;
};

// Module definitions

#include "modulesystem.h"

template<typename Type>
class GlobalUIModule;
typedef GlobalModule<IUIManager> GlobalUIManagerModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IUIManager> GlobalUIManagerModuleRef;

// This is the accessor for the event manager
inline IUIManager& GlobalUIManager() {
	return GlobalUIManagerModule::getTable();
}

#endif /*INCLUDE_UIMANAGER_H_*/
