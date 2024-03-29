#pragma once

#include "imodule.h"
#include "ui/imenu.h"

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
		SECTION_SELECTION_GROUPS, // Selection Groups
		SECTION_FILTER,	// Filter selectors
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
	static module::InstanceReference<ui::IOrthoContextMenu> _reference(MODULE_ORTHOCONTEXTMENU);
	return _reference;
}
