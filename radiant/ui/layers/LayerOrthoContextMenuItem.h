#pragma once

#include "imenu.h"
#include <memory>
#include "LayerContextMenu.h"
#include "wxutil/menu/IconTextMenuItem.h"

namespace ui
{

// A menu item which can be packed into the OrthoContextMenu
class LayerOrthoContextMenuItem :
	public IMenuItem,
	public wxutil::IconTextMenuItem
{
private:
	// Function object for the submenus
	LayerContextMenu::OnSelectionFunc _func;

	// The submenu (carrying the layer names)
	// will be deallocated by wxWidgets on shutdown
	LayerContextMenu* _submenu;

public:
	LayerOrthoContextMenuItem(const std::string& caption,
							  LayerContextMenu::OnSelectionFunc callback);

	~LayerOrthoContextMenuItem();

	// IMenuItem implementation
	wxMenuItem* getMenuItem();
	void execute();
	bool isSensitive();
	void preShow();

	// Gets called by the items in the submenus
	static void AddToLayer(int layerID);
	static void MoveToLayer(int layerID);
	static void RemoveFromLayer(int layerID);
};
typedef std::shared_ptr<LayerOrthoContextMenuItem> LayerOrthoContextMenuItemPtr;

} // namespace
