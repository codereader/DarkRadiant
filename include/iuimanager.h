#ifndef INCLUDE_UIMANAGER_H_
#define INCLUDE_UIMANAGER_H_

#include <string>
#include "generic/constant.h"

// Forward declarations
typedef struct _GtkWidget GtkWidget;

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

	/** greebo: Requests the menu bar with the given name
	 */
	virtual GtkWidget* getMenu(const std::string& name);

	/** greebo: Adds a menuitem to the given path.
	 */
	virtual void addMenuItem(const std::string& menuPath, 
							 const std::string& caption, 
					 		 const std::string& eventName) = 0;
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
