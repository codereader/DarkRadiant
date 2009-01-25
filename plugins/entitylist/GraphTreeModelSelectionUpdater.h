#ifndef GRAPHTREEMODELSELECTIONUPDATER_H_
#define GRAPHTREEMODELSELECTIONUPDATER_H_

#include "scenelib.h"
#include <gtk/gtktreeselection.h>
#include "GraphTreeModel.h"

namespace ui {

class GraphTreeModelSelectionUpdater :
	public scene::NodeVisitor
{
	GraphTreeModel& _model;
	GtkTreeSelection* _selection;
public:
	GraphTreeModelSelectionUpdater(GraphTreeModel& model, GtkTreeSelection* selection) :
		_model(model),
		_selection(selection)
	{}
	
	bool pre(const scene::INodePtr& node) {
		const GraphTreeNodePtr& gtNode = _model.find(node);
		
		if (gtNode == NULL) {
			return true;
		}
		
		if (Node_isSelected(node)) {
			gtk_tree_selection_select_iter(_selection, gtNode->getIter());
		}
		else {
			gtk_tree_selection_unselect_iter(_selection, gtNode->getIter());
		}
		
		return true;
	}
};

} // namespace ui

#endif /*GRAPHTREEMODELSELECTIONUPDATER_H_*/
