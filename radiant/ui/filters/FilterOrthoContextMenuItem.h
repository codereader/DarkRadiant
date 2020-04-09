#pragma once

#include <memory>

#include "imenu.h"
#include "ifilter.h"

#include "FilterContextMenu.h"
#include "wxutil/menu/IconTextMenuItem.h"

namespace ui
{

// A menu item which can be packed into the OrthoContextMenu
class FilterOrthoContextMenuItem :
	public IMenuItem,
	public wxutil::IconTextMenuItem
{
private:
	// Function object for the submenus
	FilterContextMenu::OnSelectionFunc _func;

	// The submenu (carrying the layer names)
	// will be deallocated by wxWidgets on shutdown
	FilterContextMenu* _submenu;

public:
	FilterOrthoContextMenuItem(const std::string& caption,
		const FilterContextMenu::OnSelectionFunc& callback);

	~FilterOrthoContextMenuItem();

	// IMenuItem implementation
	wxMenuItem* getMenuItem() override;
	void execute() override;
	bool isSensitive() override;
	void preShow() override;

	// Gets called by the items in the submenus
	static void SelectByFilter(const std::string& filterName);
	static void DeselectByFilter(const std::string& filterName);
};

} // namespace
