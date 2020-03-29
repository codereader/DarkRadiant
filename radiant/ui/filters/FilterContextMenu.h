#pragma once

#include <functional>
#include <map>
#include <memory>

#include "ifilter.h"

#include <wx/menu.h>

namespace ui
{

class FilterContextMenu :
	public wxMenu,
	public IFilterVisitor
{
public:
	// The function to be called on menu selection, the name of the
	// selected filter is passed along.
	typedef std::function<void(const std::string& filterName)> OnSelectionFunc;

private:
	OnSelectionFunc _onSelection;

	typedef std::map<int, std::string> MenuItemIdToLayerMapping;
	MenuItemIdToLayerMapping _menuItemMapping;

public:
	FilterContextMenu(OnSelectionFunc& onSelection);

	// IFilterVisitor implementation
	void visit(const std::string& filterName) override;

	// Loads filer names into the menu, clears existing items first
	void populate();

private:
	// wx Callback for menu selections
	void onActivate(wxCommandEvent& ev);
};

} // namespace ui
