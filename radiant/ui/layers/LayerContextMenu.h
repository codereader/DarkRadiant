#ifndef LAYERCONTEXTMENU_H_
#define LAYERCONTEXTMENU_H_

#include <boost/function.hpp>

#include "gtkutil/ifc/Widget.h"
#include "layers/LayerSystem.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkMenuItem GtkMenuItem;

namespace ui {

class LayerContextMenu :
	public scene::LayerSystem::Visitor,
	public gtkutil::Widget
{
public:
	// The function to be called on menu selection, the ID of the
	// selected layer is passed along.
	typedef boost::function<void(int layer)> OnSelectionFunc;

private:
	OnSelectionFunc _onSelection;
	
	GtkWidget* _menu;

	typedef std::map<std::string, int> SortedLayerMap;
	SortedLayerMap _sortedLayers;

public:
	LayerContextMenu(OnSelectionFunc& onSelection);

	// scene::LayerSystem::Visitor implementation
	void visit(int layerID, std::string layerName);

protected:
   // Widget implementation
   GtkWidget* _getWidget() const 
   {
		return _menu;
   }

private:
	// Creates the menu items
	void createMenuItems();

	// GTK Callback for menu selections
	static void onClick(GtkMenuItem* menuitem, LayerContextMenu* self);
};
typedef boost::shared_ptr<LayerContextMenu> LayerContextMenuPtr;

} // namespace ui

#endif /* LAYERCONTEXTMENU_H_ */
