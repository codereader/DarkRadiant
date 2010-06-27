#ifndef _IORTHOCONTEXT_MENU_H_
#define _IORTHOCONTEXT_MENU_H_

#include "imodule.h"

typedef struct _GtkWidget GtkWidget;

namespace ui
{

/**
 * Any element packed into the OrthoContextMenu should derive from this class.
 * A menu item must provide methods to test for sensibility and visibility,
 * which will be invoked each time before the menu is presented to the user.
 */
class IOrthoContextMenuItem
{
public:
	/**
	 * Callback to check whether this item should be visible in the menu.
	 * This is invoked by the OrthoContextMenu before presenting itself to the user.
	 */
	virtual bool isVisible() = 0; 

	/**
	 * Callback to check whether this item should be sensitive in the menu.
	 * Insensitive items are displayed as "greyed out".
	 * This is invoked by the OrthoContextMenu before presenting itself to the user.
	 */
	virtual bool isSensitive() = 0;

	/**
	 * Returns the widget needed for packing this into the menu.
	 */
	virtual GtkWidget* getWidget() = 0;
};
typedef boost::shared_ptr<IOrthoContextMenuItem> IOrthoContextMenuItemPtr;

class IOrthoContextMenu :
	public RegisterableModule
{
public:
	// The section enum specifies where menu item should be listed under
	// It's possible to add new sections > SECTION_USER
	enum Section
	{
		SECTION_CREATE,	// Create Entity, Create Speaker, etc.
		SECTION_ACTION, // Make Visportals
		SECTION_LAYER,	// Layer operations
		SECTION_USER = 100,
	};

	/** 
	 * Adds a new ortho context menu item in the given section.
	 *
	 * @section: an integer value referring to the Section enum above.
	 * It's possible to specify new sections by passing values > SECTION_USER.
	 * Sections are visually separated by horizontal lines in the menu.
	 */
	virtual void addItem(const IOrthoContextMenuItemPtr& item, int section) = 0;

	/**
	 * Removes a certain item from the menus.
	 */
	virtual void removeItem(const IOrthoContextMenuItemPtr& item) = 0;
};

} // namespace

const char* const MODULE_ORTHOCONTEXTMENU = "OrthoContextMenu";

inline ui::IOrthoContextMenu& GlobalOrthoContextMenu()
{
	// Cache the reference locally
	static ui::IOrthoContextMenu& _menu(
		*boost::static_pointer_cast<ui::IOrthoContextMenu>(
			module::GlobalModuleRegistry().getModule(MODULE_ORTHOCONTEXTMENU)
		)
	);
	return _menu;
}

#endif /* _IORTHOCONTEXT_MENU_H_ */
