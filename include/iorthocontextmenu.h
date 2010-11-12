#ifndef _IORTHOCONTEXT_MENU_H_
#define _IORTHOCONTEXT_MENU_H_

#include "imodule.h"
#include "imenu.h"

typedef struct _GtkWidget GtkWidget;

namespace ui
{

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
	virtual void addItem(const IMenuItemPtr& item, int section) = 0;

	/**
	 * Removes a certain item from the menus.
	 */
	virtual void removeItem(const IMenuItemPtr& item) = 0;
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
