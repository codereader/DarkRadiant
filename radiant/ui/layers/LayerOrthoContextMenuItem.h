#ifndef _LAYER_ORTHO_CONTEXT_MENU_H_
#define _LAYER_ORTHO_CONTEXT_MENU_H_

#include "imenu.h"
#include "LayerContextMenu.h"
#include "gtkutil/IconTextMenuItem.h"

namespace ui
{

// A menu item which can be packed into the OrthoContextMenu
class LayerOrthoContextMenuItem :
	public IMenuItem,
	public gtkutil::IconTextMenuItem
{
private:
	// Function object for the submenus
	LayerContextMenu::OnSelectionFunc _func;

	// The submenu (carrying the layer names)
	LayerContextMenuPtr _submenu;

public:
	LayerOrthoContextMenuItem(const std::string& caption,
							  LayerContextMenu::OnSelectionFunc callback);

	// IMenuItem implementation
	Gtk::MenuItem* getWidget();
	void execute();
	bool isSensitive();
	void preShow();

	// Gets called by the items in the submenus
	static void AddToLayer(int layerID);
	static void MoveToLayer(int layerID);
	static void RemoveFromLayer(int layerID);
};
typedef boost::shared_ptr<LayerOrthoContextMenuItem> LayerOrthoContextMenuItemPtr;

} // namespace

#endif /* _LAYER_ORTHO_CONTEXT_MENU_H_ */

