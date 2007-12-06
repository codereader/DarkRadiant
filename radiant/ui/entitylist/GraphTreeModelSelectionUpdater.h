#ifndef GRAPHTREEMODELSELECTIONUPDATER_H_
#define GRAPHTREEMODELSELECTIONUPDATER_H_

#include "scenelib.h"
#include <gtk/gtktreeselection.h>
#include "GraphTreeModel.h"

namespace ui {

class GraphTreeModelSelectionUpdater :
	public scene::Graph::Walker
{
	GraphTreeModel& _model;
	GtkTreeSelection* _selection;
public:
	GraphTreeModelSelectionUpdater(GraphTreeModel& model, GtkTreeSelection* selection) :
		_model(model),
		_selection(selection)
	{}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		const GraphTreeNodePtr& node = _model.find(instance);
		
		if (node == NULL) {
			return true;
		}
		
		if (Instance_isSelected(instance)) {
			gtk_tree_selection_select_iter(_selection, node->getIter());
		}
		else {
			gtk_tree_selection_unselect_iter(_selection, node->getIter());
		}
		
		return true;
	}
};

} // namespace ui

#endif /*GRAPHTREEMODELSELECTIONUPDATER_H_*/
