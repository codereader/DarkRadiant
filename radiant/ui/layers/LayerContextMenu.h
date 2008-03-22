#ifndef LAYERCONTEXTMENU_H_
#define LAYERCONTEXTMENU_H_

#include <boost/function.hpp>

#include "layers/LayerSystem.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkMenuItem GtkMenuItem;

namespace ui {

class LayerContextMenu :
	public scene::LayerSystem::Visitor
{
public:
	// The function to be called on menu selection, the ID of the
	// selected layer is passed along.
	typedef boost::function<void(int layer)> OnSelectionFunc;

private:
	OnSelectionFunc _onSelection;
	
	GtkWidget* _menu;

public:
	LayerContextMenu(OnSelectionFunc& onSelection);

	// Operator cast to a GtkWidget*
	operator GtkWidget*();

	// scene::LayerSystem::Visitor implementation
	void visit(int layerID, std::string layerName);

private:
	// GTK Callback for menu selections
	static void onClick(GtkMenuItem* menuitem, LayerContextMenu* self);
};
typedef boost::shared_ptr<LayerContextMenu> LayerContextMenuPtr;

} // namespace ui

#endif /* LAYERCONTEXTMENU_H_ */
